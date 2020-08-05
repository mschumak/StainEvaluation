/*=============================================================================
 *
 *  Copyright (c) 2020 Sunnybrook Research Institute
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 *=============================================================================*/

#include "StainEvaluation.h"

#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>
#include <filesystem> //Requires C++17

//Sedeen required headers
#include "Algorithm.h"
#include "Geometry.h"
#include "Global.h"
#include "Image.h"
#include "image/io/Image.h"
#include "image/tile/Factory.h"

// Poco header needed for the macros below 
#include <Poco/ClassLibrary.h>

// Declare that this object has AlgorithmBase subclasses
//  and declare each of those sub-classes
POCO_BEGIN_MANIFEST(sedeen::algorithm::AlgorithmBase)
POCO_EXPORT_CLASS(sedeen::algorithm::StainEvaluation)
POCO_END_MANIFEST

namespace sedeen {
namespace algorithm {

//Constructor
StainEvaluation::StainEvaluation()
    : m_displayArea(),
    m_imageList(), 
    m_maskThreshold(),
    m_saveCroppedImageFileAs(),
    m_saveMaskedImageFileAs(),

    //Member data
    m_maskImage(),
    m_sourceImage(),
    m_maskImageProperties(),
    m_sourceImageProperties(),
    m_maskSourceIntersectionRect(),

    m_testImage(),
    m_refImage(),
    m_testImageProperties(),
    m_refImageProperties(),

    //Output objects
    m_result(),
    m_outputText(),
    m_overlayResult(),
    m_report(""),
    m_maskThresholdDefaultVal(20.0),
    m_maskThresholdMaxVal(255.0),
    m_pixelWarningThreshold(1e8), //100,000,000 pixels, ~400 MB
    m_Mask_factory(nullptr)
{
    //List of the file extensions that should be included in the save dialog windows
    m_saveFileExtensionText.push_back("tif");
    m_saveFileExtensionText.push_back("png");
    m_saveFileExtensionText.push_back("bmp");
    m_saveFileExtensionText.push_back("gif");
    m_saveFileExtensionText.push_back("jpg");
    //TODO: enable saving a whole slide image
    //m_saveFileExtensionText.push_back("svs");
}//end  constructor

 //Destructor
StainEvaluation::~StainEvaluation() {
}

void StainEvaluation::init(const image::ImageHandle& image) {
    if (isNull(image)) return;
    // bind algorithm members to UI and initialize their properties

    // Bind system parameter for current view
    m_displayArea = createDisplayAreaParameter(*this);
    // Bind system parameter for the image list
    m_imageList = createImageListParameter(*this);

    //The threshold to apply to the mask image
    m_maskThreshold = createDoubleParameter(*this,
        "Mask Threshold Value",   // Widget label
        "Threshold value to apply to the second image to create a mask of which pixels in the source to retain.",   // Widget tooltip
        m_maskThresholdDefaultVal, // Initial value
        0.0,                       // minimum value
        m_maskThresholdMaxVal,     // maximum value
        1.0,                       // step size
        false);

    //Allow the user to choose where to save the cropped and masked image files (separately)
    sedeen::file::FileDialogOptions croppedSaveOptions = defineSaveFileDialogOptions("Save cropped image as...");
    m_saveCroppedImageFileAs = createSaveFileDialogParameter(*this, "Save Cropped Image As...",
        "Save the cropped source image, cropped to the bounds of the intersection of the source and mask images.",
        croppedSaveOptions, true);

    sedeen::file::FileDialogOptions maskedSaveOptions = defineSaveFileDialogOptions("Save masked image as...");
    m_saveMaskedImageFileAs = createSaveFileDialogParameter(*this, "Save Masked Image As...",
        "Save the image with the mask and cropping applied.",
        maskedSaveOptions, true);

    // Bind output results
    m_outputText = createTextResult(*this, "Text Result");
    m_result = createImageResult(*this, "StainEvaluationResult");
    m_overlayResult = createOverlayResult(*this);

}//end init







void StainEvaluation::run() {
    //Define an empty report string
    std::string report = std::string();

    // Has display area changed
    bool display_changed = m_displayArea.isChanged();
    // Has the list of images changed
    bool image_list_changed = m_imageList.isChanged();

    //Check how many images are loaded. 
    //If it's not two, send a report and end processing.
    if (m_imageList.count() != 2) {
        report.append("This plugin requires exactly two images to be loaded. ");
        report.append("Please load a source image and a mask image in Sedeen. ");
        report.append("Click on the SOURCE image to highlight it. ");
        report.append("Check that the SOURCE image location is in the Image text box at the top of the Analysis Manager.");

        //These will be for the comparison!
        //report.append("Please load a test image and a reference image in Sedeen. ");
        //report.append("Click on the test image to highlight it. ");
        //report.append("Check that the test image location is in the Image text box at the top of the Analysis Manager.");
        
        //Send report, return from run method
        m_outputText.sendText(report);
        return;
    }

    //This is for applying the mask image to a source (DAPI on unseparated image)
    bool pipeline_changed = buildApplyMaskPipeline();

    //Add the properties of the two images to the output report
    report.append("Mask image properties:\n");
    report.append(generateImagePropertiesReport(m_maskImageProperties));
    report.append("\nSource image properties:\n");
    report.append(generateImagePropertiesReport(m_sourceImageProperties));

    //For later: test and reference comparison
    //Add the properties of the two images to the output report
    //report.append("Test image properties:\n");
    //report.append(generateImagePropertiesReport(m_testImageProperties));
    //report.append("\nReference image properties:\n");
    //report.append(generateImagePropertiesReport(m_refImageProperties));


    if (display_changed 
        || image_list_changed 
        || pipeline_changed   ) {
        //Check that the file locations for the cropped and masked images can be written to
        //Get the full path file names from the file dialog parameters

        //Check that the location specified for the cropped image can be written to
        //Get the full path file name from the file dialog parameter
        std::string croppedFilePath;
        sedeen::algorithm::parameter::SaveFileDialog::DataType croppedDialogDataType = this->m_saveCroppedImageFileAs;
        croppedFilePath = croppedDialogDataType.getFilename();
        bool croppedFileCheck = checkImageSaveProperties(croppedFilePath, report, "cropped");
        if (!croppedFileCheck) { return; }

        //Check that the location specified for the masked image can be written to
        //Get the full path file name from the file dialog parameter
        std::string maskedFilePath;
        sedeen::algorithm::parameter::SaveFileDialog::DataType maskedDialogDataType = this->m_saveMaskedImageFileAs;
        maskedFilePath = maskedDialogDataType.getFilename();
        bool maskedFileCheck = checkImageSaveProperties(maskedFilePath, report, "masked");
        if (!maskedFileCheck) { return; }


        //This is where the magic happens.
        //if (nullptr != m_Mask_factory) {
        //m_result.update(m_Mask_factory, m_displayArea, *this);
        //}

        // Update the output text report
        if (false == askedToStop()) {
            //Save the cropped image to file (file error checks performed above)
            bool croppedSaveResult = SaveCroppedImageToFile(image()->getFactory(), croppedFilePath, m_maskSourceIntersectionRect); //THIS DOES NOT WORK
            //Check that the cropped image was saved successfully
            std::stringstream sc;
            if (croppedSaveResult) {
                sc << std::endl << "Cropped image saved as " << croppedFilePath << std::endl;
                report.append(sc.str());
            }
            else {
                sc << std::endl << "Saving the cropped image failed. Please check the file name and directory permissions." << std::endl;
                report.append(sc.str());
            }

            //Save the masked image to file (file error checks performed above)
            bool maskedSaveResult = SaveCroppedImageToFile(image()->getFactory(), maskedFilePath, m_maskSourceIntersectionRect); //THIS DOES NOT WORK  
            //Check that the masked image was saved successfully
            std::stringstream sm;
            if (maskedSaveResult) {
                sm << std::endl << "Masked image saved as " << maskedFilePath << std::endl;
                report.append(sm.str());
            }
            else {
                sm << std::endl << "Saving the masked image failed. Please check the file name and directory permissions." << std::endl;
                report.append(sm.str());
            }

            //Send the final report to the results window
            m_outputText.sendText(report);
        }
    }//if display or pipeline changed

    // Ensure we run again after an abort
    if (askedToStop()) {
        //reset the factory
        m_Mask_factory.reset();
    }
}//end run


PointF StainEvaluation::GetImageCenterFromProperties(const ImageProperties &ip) {
    //Sometimes the center() stored in the transform is (0,0). To deal with that:
    //If there are non-zero values for the center coordinates in the transform, use those
    //If not, use half the image size multiplied by the pixel size as the center coordinates
    SRTTransform tr = ip.sedeen_transform;
    SizeF pixelSize = ip.image_pixel_size;
    //x
    double x = (tr.center().getX() == 0.0)
        ? (pixelSize.width() * static_cast<double>(ip.image_size.width()) / 2.0)
        : tr.center().getX();
    //y
    double y = (tr.center().getY() == 0.0)
        ? (pixelSize.height() * static_cast<double>(ip.image_size.height()) / 2.0)
        : tr.center().getY();

    PointF theCenter = PointF(x, y);
    return theCenter;
}//end GetImageCenterFromProperties


PointF StainEvaluation::CalculateCenterDifference(const ImageProperties &initial, const ImageProperties &final) {
    //The coordinates of the image centers are in units of um, so pixels*pixel_size/2
    PointF initialCenter = GetImageCenterFromProperties(initial);
    PointF finalCenter = GetImageCenterFromProperties(final);
    SizeF finalPixelSize = final.image_pixel_size;

    //Get differences between center coordinate values
    double xCenterDiff = (finalCenter.getX() - initialCenter.getX());
    double yCenterDiff = (finalCenter.getY() - initialCenter.getY());

    PointF centerDiff = PointF(xCenterDiff / finalPixelSize.width(), yCenterDiff / finalPixelSize.height());
    return centerDiff;
}//end CalculateCenterDifference



PointF ChangeReferenceFramerrr(const PointF &pf,
    const ImageProperties &initial, const ImageProperties &final) {
    //The coordinates of the image centers are in units of um, so pixels*pixel_size/2
    SRTTransform initialTransform = initial.sedeen_transform;
    SRTTransform finalTransform = final.sedeen_transform;
    SizeF initialPixelSize = initial.image_pixel_size;
    SizeF finalPixelSize = final.image_pixel_size;

    //x
    double xFinalCenter = finalTransform.center().getX();
    double xInitialCenter = initialTransform.center().getX();
    //y
    double yFinalCenter = finalTransform.center().getY();
    double yInitialCenter = initialTransform.center().getY();

    //Get differences between center coordinate values
    double xCenterDiff = (xFinalCenter - xInitialCenter);
    double yCenterDiff = (yFinalCenter - yInitialCenter);
    //Convert the initial point coordinates to the coordinate system of the final image
    double xOutPoint = (initialPixelSize.width() * pf.getX() + xCenterDiff) / finalPixelSize.width();
    double yOutPoint = (initialPixelSize.height() * pf.getY() + yCenterDiff) / finalPixelSize.height();
    //Assign output point coordinates to the elements of a PointF
    PointF outPoint = PointF(xOutPoint, yOutPoint);
    return outPoint;
}//end ChangeReferenceFrame (PointF)



Polygon StainEvaluation::TransformPolygon(const Polygon &poly, const ImageProperties &initial, const ImageProperties &final) {
    if (poly.isNull()) { return Polygon(); }
    //Define an identity transform
    SRTTransform identityTransform(0., 0., 1., 1., 0., 0., 0.); //(translation, scale, rotation (deg), center of rotation)

    //Work out how to deal with user-specified changes to the pixel spacing LAST.
    SizeF initialTransformSpacing = initial.tr_pixel_spacing;
    SizeF finalTransformSpacing = final.tr_pixel_spacing;

    //Transforms and the stored pixel size for the images
    SRTTransform initialTransform = initial.sedeen_transform;
    SRTTransform finalTransform = final.sedeen_transform;
    SizeF initialImagePixelSize = initial.image_pixel_size;
    SizeF finalImagePixelSize = final.image_pixel_size;

    //Locations of the image centers in their own coordinate systems
    PointF initialImageCenter = GetImageCenterFromProperties(initial);
    PointF finalImageCenter = GetImageCenterFromProperties(final);

    //My nomenclature as compared with ExportTransformedROI:
    //"target" image in ExportTransformedROI is my "final" image
    //"source" image in ExportTransformedROI is my "initial" image

    //Define the new transforms to be applied to the Polygon
    //inverse of the transform applied to the final image space
    SRTTransform fSpaceTransform(identityTransform);
    //forward transform of the initial image space, converted to final image space units
    SRTTransform iSpaceTransform(identityTransform);

    //What to apply to the iSpaceTransform, which is the initial image space transform
    PointF iBaseTranslation = initialTransform.translation();
    PointF iBaseCenter = initialTransform.center();
    SizeF iScale = initialTransform.scale();
    double iRotation = initialTransform.rotation();
    //Copy the scale factors
    iSpaceTransform.setScale(iScale);
    //Copy the rotation angle
    iSpaceTransform.setRotation(iRotation);
    //Rescale the translation
    //PointF iSpaceTranslation = PointF(iBaseTranslation.getX() / finalImagePixelSize.width(),
    //    iBaseTranslation.getY() / finalImagePixelSize.height());

    PointF iSpaceTranslation = PointF(iBaseTranslation.getX()  / finalImagePixelSize.width() - (iScale.width() - 1.0)*iBaseCenter.getX(),
        //+ finalImageCenter.getX()*iScale.width() / finalImagePixelSize.width(),
        
        iBaseTranslation.getY() / finalImagePixelSize.height() - (iScale.height() - 1.0)*iBaseCenter.getY()  );
    iSpaceTransform.setTranslation(iSpaceTranslation);


    //s_transform.setCenter(s_transform.center().getX() / s_spacing.width(), 
    //    s_transform.center().getY() / s_spacing.height());


    //What to apply to the fSpaceTransform, which is the final image space transform
    PointF fBaseTranslation = finalTransform.translation();
    PointF fBaseCenter = finalTransform.center();
    SizeF fScale = finalTransform.scale();
    double fRotation = finalTransform.rotation();
    //Copy the scale factors
    fSpaceTransform.setScale(fScale);
    //Copy the rotation angle
    fSpaceTransform.setRotation(fRotation);

    //Rescale the translation
    //PointF fSpaceTranslation = PointF(fBaseTranslation.getX() / finalImagePixelSize.width(),
    //    fBaseTranslation.getY() / finalImagePixelSize.height());

    PointF fSpaceTranslation = PointF( fBaseTranslation.getX() / finalImagePixelSize.width(),
         fBaseTranslation.getY() / finalImagePixelSize.height());

    fSpaceTransform.setTranslation(fSpaceTranslation);




    //Apply the new transforms to the input Polygon
    Polygon tempPolygon(poly);
    //Inverse of the fSpaceTransform
    tempPolygon.transform(fSpaceTransform, sedeen::TransformDirection::Inverse);
    //Apply the iSpaceTransform (initial image transform in final image space units)
    tempPolygon.transform(iSpaceTransform, sedeen::TransformDirection::Forward);
    return tempPolygon;
}//end TransformPolygon




bool StainEvaluation::buildApplyMaskPipeline() {
    using namespace image::tile;
    bool pipeline_changed = false;

    //Re-check the number of loaded images
    if (m_imageList.count() != 2) {
        return false;
    }

    bool doProcessing = false;
    if (pipeline_changed
        || m_displayArea.isChanged()
        || m_imageList.isChanged()
        || m_maskThreshold.isChanged()
        || m_saveCroppedImageFileAs.isChanged()
        || m_saveMaskedImageFileAs.isChanged()
        || (nullptr == m_Mask_factory) )
    {

        //m_maskImage;
        //m_sourceImage;

        //m_maskImageProperties;
        //m_sourceImageProperties;


        //The SOURCE image should be the one in the Image textbox, and 
        //so should be pointed to by image(). Get the properties of it and
        //the other loaded image (the MASK image) and assign them to member variables.

        int sourceImageIndex = m_imageList.indexOf(image());
        int maskImageIndex = 1 - m_imageList.indexOf(image());

        //Mask image info, properties, and Image object pointer (workaround)
        ImageInfo maskImageInfo = m_imageList.info(maskImageIndex);
        std::string maskImageLocation = maskImageInfo.location;
        //sedeen::SRTTransform maskImageTransform = maskImageInfo.transform;
        auto maskImageOpener = image::createImageOpener();
        m_maskImage = maskImageOpener->open(file::Location(maskImageLocation));
        
        //If an image currently open in Sedeen cannot be re-opened, throw error
        if (!m_maskImage) {
            throw std::runtime_error("Could not open the mask image: " + file::Location(maskImageLocation).getFilename());
        }
        //TEMP approach to this
        //If one exists, open the session file for the image
        //The problem is that new changes to the Pixel Spacing boxes are not stored here yet
        sedeen::Session maskSession = sedeen::Session(maskImageLocation);
        maskSession.loadFromFile();
        //
        //The pixel spacing as set in the Transform box. Stored in the Session xml file for the image
        sedeen::SizeF maskTrSpacing = maskSession.getPixelSize();
        //Fill the ImageProperties struct for the test image
        m_maskImageProperties = GetImageProperties(m_maskImage, &maskImageInfo, maskTrSpacing);

        //Source image info, properties, and Image object pointer (workaround)
        ImageInfo sourceImageInfo = m_imageList.info(sourceImageIndex);
        std::string sourceImageLocation = sourceImageInfo.location;
        //sedeen::SRTTransform sourceImageTransform = sourceImageInfo.transform;
        auto sourceImageOpener = image::createImageOpener();
        m_sourceImage = sourceImageOpener->open(file::Location(sourceImageLocation));
        //If an image currently open in Sedeen cannot be re-opened, throw error
        if (!m_sourceImage) {
            throw std::runtime_error("Could not open the source image: " + file::Location(sourceImageLocation).getFilename());
        }
        //TEMP approach to this
        //If one exists, open the session file for the image
        //The problem is that new changes to the Pixel Spacing boxes are not stored here yet
        sedeen::Session sourceSession = sedeen::Session(sourceImageLocation);
        sourceSession.loadFromFile();
        //
        //The pixel spacing as set in the Transform box. Stored in the Session xml file for the image
        sedeen::SizeF sourceTrSpacing = sourceSession.getPixelSize();
        //Fill the ImageProperties struct for the source image
        m_sourceImageProperties = GetImageProperties(m_sourceImage, &sourceImageInfo, sourceTrSpacing);

        //Get the containing rectangles of the images with double-precision element data
        sedeen::RectF maskContainingRectF = sedeen::RectF(sedeen::image::rect(m_maskImage, 0));
        sedeen::RectF sourceContainingRectF = sedeen::RectF(sedeen::image::rect(m_sourceImage, 0));
                
        //Create Polygons from the mask and source Rect objects
        Polygon maskPolygon = RectFToPolygon(maskContainingRectF);
        Polygon sourcePolygon = RectFToPolygon(sourceContainingRectF);

        //Change the coordinate reference of the mask polygon to the reference frame of the source image
        Polygon reframedMaskPolygon = ChangeReferenceFrame(maskPolygon, m_maskImageProperties, m_sourceImageProperties);

        //The source image polygon will always stay in the coordinate system of the source image,
        //so no transform needs to be applied to it. Instead, apply an inverse transform to the mask image polygon
        //before applying its transform
        Polygon trMaskPolygon = TransformPolygon(reframedMaskPolygon, m_maskImageProperties, m_sourceImageProperties);


        //Draw the mask polygon
        auto maskGraphicStyle = GraphicStyle();
        maskGraphicStyle.setLabel("Mask image border");
        Pen maskPen = Pen(sedeen::RGBColor(255, 255, 0), 3, LineStyle::Dashed);
        maskGraphicStyle.setPen(maskPen);
        m_overlayResult.drawPolygon(trMaskPolygon, maskGraphicStyle, "Mask image border", std::string());

        //Draw the source polygon
        auto sourceGraphicStyle = GraphicStyle();
        sourceGraphicStyle.setLabel("Source image border");
        Pen sourcePen = Pen(sedeen::RGBColor(0, 255, 255), 3, LineStyle::Dashed);
        sourceGraphicStyle.setPen(sourcePen);
        m_overlayResult.drawPolygon(sourcePolygon, sourceGraphicStyle, "Source image border", std::string());

        
        //Find the intersection of the two rectangles
        m_maskSourceIntersectionRect = Rect();   //sedeen::intersection(maskContainingRectF, sourceContainingRectF);


        //Draw the intersection
        




        //If the intersection is empty, set the mask factory equal to the source factory
        bool intersectionEmpty = sedeen::isEmpty(m_maskSourceIntersectionRect);
        if (intersectionEmpty) {
            //Set the member factory equal to the source factory
            m_Mask_factory = image()->getFactory();
            pipeline_changed = false;
        }
        else {
            //Crop
            //Threshold
            //Resample
            //Mask
            //Save both cropped

            //Pixel-to-pixel operations

            //Constrain processing to the intersection rectangle
            auto source_factory = image()->getFactory();
            //auto constrained_factory = std::make_shared<RegionFactory>(source_factory, m_maskSourceIntersectionRect);




            //auto display_resolution = getDisplayResolution(image(), m_displayArea);

            //Get the Behavior value from the m_retainment
            //Scale down the threshold to create more precision
            //auto threshold_kernel =
            //    std::make_shared<image::tile::ODThresholdKernel>(m_threshold / 100.0,
            //    behaviorVal, theWeights);

            // Create a Factory for the composition of these Kernels
            //auto non_cached_factory =
            //    std::make_shared<FilterFactory>(source_factory, threshold_kernel);

            // Wrap resulting Factory in a Cache for speedy results
            //m_ODThreshold_factory =
            //    std::make_shared<Cache>(non_cached_factory, RecentCachePolicy(30));




            // Wrap resulting Factory in a Cache for speedy results
            m_Mask_factory = image()->getFactory();   //std::make_shared<Cache>(constrained_factory, RecentCachePolicy(30));

            pipeline_changed = true;
        }  
    }//end if parameter values changed

    return pipeline_changed;
}//end buildApplyMaskPipeline







ImageProperties StainEvaluation::GetImageProperties(const image::ImageHandle& im, sedeen::algorithm::ImageInfo *imageinfo,
    const sedeen::SizeF &trSpacing /*= sedeen::SizeF(1.0, 1.0)*/)
{
    ImageProperties imProps;
    if (imageinfo == nullptr) {
        return imProps;
    }

    //auto imageinfo = image_list_.info(index);     
    //image_list_.indexOf(image()));
    imProps.sedeen_transform = imageinfo->transform;
    imProps.opacity = imageinfo->opacity;
    imProps.visibility = imageinfo->visible;
    imProps.location = imageinfo->location;

    //The pixel spacing as set in the Transform box.
    imProps.tr_pixel_spacing = trSpacing;

    //Get the ColorSpace for the image
    auto source_factory = im->getFactory();
    sedeen::ColorSpace color_space = source_factory->getColorSpace();
    imProps.color_model = color_space.colorModel();
    imProps.pixel_type = color_space.channelType();

    auto dataServer = createDataServer(im);
    imProps.nlevels = dataServer->getNumLevels();

    int im_width = im->getMetaData()->get(image::IntegerTags::IMAGE_X_DIMENSION, 0);  // pixel
    int im_height = im->getMetaData()->get(image::IntegerTags::IMAGE_Y_DIMENSION, 0);
    imProps.image_size = sedeen::Size(im_width, im_height);

    double x_pixel_size = 1.;	//default val, um
    double y_pixel_size = 1.;
    if (im->getMetaData()->has(image::DoubleTags::PIXEL_SIZE_X)) {
        x_pixel_size = im->getMetaData()->get(image::DoubleTags::PIXEL_SIZE_X, 0); //um
    }
    if (im->getMetaData()->has(image::DoubleTags::PIXEL_SIZE_Y)) {
        y_pixel_size = im->getMetaData()->get(image::DoubleTags::PIXEL_SIZE_Y, 0); //um
    }
    imProps.image_pixel_size = sedeen::SizeF(x_pixel_size, y_pixel_size);

    return imProps;
}//end GetImageProperties



const std::string StainEvaluation::generateImagePropertiesReport(const ImageProperties &ip) {
    sedeen::Size   image_size = ip.image_size;
    sedeen::SizeF  image_pixel_size = ip.image_pixel_size;

    sedeen::SRTTransform tr = ip.sedeen_transform;
    sedeen::PointF trcenter = tr.center();
    sedeen::PointF trtranslation = tr.translation();
    sedeen::SizeF  trscale = tr.scale();
    sedeen::SizeF tr_pixel_spacing = ip.tr_pixel_spacing;

    std::stringstream ss;
    //Compose a report block with the properties of this image
    ss << "Image location: " << ip.location << std::endl;
    ss << "Number of levels: " << ip.nlevels << std::endl;
    ss << "---Image Size---" << std::endl;
    ss << "    Width: " << image_size.width() << std::endl;
    ss << "    Height: " << image_size.height() << std::endl;
    ss << "---Image Pixel Size---" << std::endl;
    ss << "    Pixel Width: " << image_pixel_size.width() << " um" << std::endl;
    ss << "    Pixel Height: " << image_pixel_size.height() << " um" << std::endl;
    ss << "---Sedeen Transform---" << std::endl;
    ss << "    Pixel Spacing (um): (" << tr_pixel_spacing.width() << ", " << tr_pixel_spacing.height() << ")" << std::endl;
    ss << "    Center: (" << trcenter.getX() << ", " << trcenter.getY() << ")" << std::endl;
    ss << "    Translation: (" << trtranslation.getX() << ", " << trtranslation.getY() << ")" << std::endl;
    ss << "    Scale: (" << trscale.width() << ", " << trscale.height() << ")" << std::endl;
    ss << "    Rotation: " << tr.rotation() << std::endl;
    ss << "Opacity: " << ip.opacity << std::endl;
    ss << "Visibility: " << ip.visibility << std::endl;
    ss << "Color model and pixel type: " << sedeen::colorDescription(sedeen::ColorSpace(ip.color_model, ip.pixel_type)) << std::endl;

    return ss.str();
}//end generateImagePropertiesReport




//image::RawImage StainEvaluation::GetDisplayImage(const image::ImageHandle& im)
//{
//    DisplayRegion display_region = m_displayArea;
//    // Get the region currently on screen
//
//    auto displayRegionScale = getDisplayScale(display_region);
//    auto imDisplayIterator = getIterator(im, display_region);
//    Point displayRegionPosition(imDisplayIterator.x(), imDisplayIterator.y());
//    auto imageDisplaySize = display_region.output_size;
//    Rect imageDisplayRect(displayRegionPosition, imageDisplaySize);
//
//    image::tile::Compositor compositor(im->getFactory());
//    auto input = compositor.getImage(imageDisplayRect, display_region.output_size);
//    return input;
//}//end GetDisplayImage








///Define the save file dialog options outside of init
sedeen::file::FileDialogOptions StainEvaluation::defineSaveFileDialogOptions(const std::string &capt /*= std::string()*/) {
    sedeen::file::FileDialogOptions theOptions;
    if (!capt.empty()) {
        theOptions.caption = capt;
    }
    else {
        theOptions.caption = "Save image as...";
    }
    //theOptions.flags = sedeen::file::FileDialogFlags:: None currently needed
    //theOptions.startDir; //no current preference
    //Define the file type dialog filter
    sedeen::file::FileDialogFilter theDialogFilter;
    theDialogFilter.name = "Image type";
    //Add extensions in m_saveFileExtensionText to theDialogFilter.extensions 
    //Note: std::copy does not work here
    for (auto it = m_saveFileExtensionText.begin(); it != m_saveFileExtensionText.end(); ++it) {
        theDialogFilter.extensions.push_back(*it);
    }
    theOptions.filters.push_back(theDialogFilter);
    return theOptions;
}//end defineSaveFileDialogOptions

double StainEvaluation::EstimateOutputImageSize(const sedeen::Rect &r) {
    double estSize;
    if (!sedeen::isEmpty(r)) {
        Rect rect = r;
        double sizeFromRect = static_cast<double>(rect.height())
            * static_cast<double>(rect.width());
        estSize = sizeFromRect;
    }
    else {
        //The given Rect is empty.
        estSize = 0.0;
    }
    return estSize;
}//end EstimateOutputImageSize

std::string StainEvaluation::EstimateImageStorageSize(const double &pix) {
    const double bytesPerPixel(4.0); //depends on file type and colour model
    const double estStorageSize = bytesPerPixel * pix;
    if (estStorageSize < 1.0) { return "0 bytes"; }
    //bytes? kB? MB? GB? TB?
    int power = static_cast<int>(std::log(estStorageSize) / std::log(1024.0));
    double val = estStorageSize / std::pow(1024.0, power*1.0);
    std::stringstream ss;
    ss << std::setprecision(3) << val << " ";
    if (power == 0) {
        ss << "bytes";
    }
    else if (power == 1) {
        ss << "kB";
    }
    else if (power == 2) {
        ss << "MB";
    }
    else if (power == 3) {
        ss << "GB";
    }
    else {
        ss << "TB";
    }
    return ss.str();
}//end EstimateImageStorageSize

bool StainEvaluation::checkImageSaveProperties(const std::string &path, const std::string &rep, const std::string &desc) {
    std::string moreReport(rep);
    const bool successVal = true;
    const bool failVal = false;
    //Is the path string blank?
    if (path.empty()) {
        moreReport.append("\nThere is no location given for where to save the ");
        moreReport.append(desc);
        moreReport.append(" image. Please enter a file name.\n");
        m_outputText.sendText(moreReport);
        return failVal;
    }
    else {
        //Does it exist or can it be created, and can it be written to?
        bool validFileCheck = checkFile(path, "w");
        if (!validFileCheck) {
            moreReport.append("The file name selected for the ");
            moreReport.append(desc);
            moreReport.append(" image cannot be written to.");
            moreReport.append(" Please choose another name, or check the permissions of the directory.");
            m_outputText.sendText(moreReport);
            return failVal;
        }
        //Does it have a valid extension? RawImage.save relies on the extension to determine save format
        std::string theExt = getExtension(path);
        int extensionIndex = findExtensionIndex(theExt);
        //findExtensionIndex returns -1 if not found
        if (extensionIndex == -1) {
            std::stringstream ss;
            ss << "The extension of the file for the ";
            ss << desc;
            ss << " image is not a valid type.The file extension must be : ";
            auto vec = m_saveFileExtensionText;
            for (auto it = vec.begin(); it != vec.end() - 1; ++it) {
                ss << (*it) << ", ";
            }
            std::string last = vec.back();
            ss << "or " << last << ". Choose a correct file type and try again." << std::endl;
            moreReport.append(ss.str());
            m_outputText.sendText(moreReport);
            return failVal;
        }
    }
    //All checks passed
    return successVal;
}//end checkImageSaveProperties

bool StainEvaluation::SaveCroppedImageToFile(std::shared_ptr<image::tile::Factory> factory, 
    const std::string &p, const sedeen::Rect &rect) {
    const bool errorVal = false;
    //Is the given Rect empty? 
    if (sedeen::isEmpty(rect)) { return errorVal; }
    //Check if Factory exists
    if (nullptr == factory) { return errorVal; }
    //Assume file type and ability to save have already been checked.
    //In RawImage::save, the used file format is defined by the file extension.
    //Supported extensions are : .tif, .png, .bmp, .gif, .jpg
    std::string outFilePath = p;
    bool imageSaved = false;
    //Access the output from the given factory
    auto outputFactory = factory;
    auto compositor = std::make_unique<image::tile::Compositor>(outputFactory);
    //Get the image at the highest resolution (level 0)
    sedeen::image::RawImage outputImage = compositor->getImage(0, rect);
    //Save the outputImage to a file at the given location
    imageSaved = outputImage.save(outFilePath);
    return imageSaved; //true on successful save, false otherwise
}//end SaveCroppedImageToFile

const std::string StainEvaluation::getExtension(const std::string &p) {
    namespace fs = std::filesystem; //an alias
    const std::string errorVal = std::string(); //empty
    //Convert the string to a filesystem::path
    fs::path filePath(p);
    //Does the filePath have an extension?
    bool hasExtension = filePath.has_extension();
    if (!hasExtension) { return errorVal; }
    //else
    fs::path ext = filePath.extension();
    return ext.string();
}//end getExtension

const int StainEvaluation::findExtensionIndex(const std::string &x) const {
    const int notFoundVal = -1; //return -1 if not found
    //This method works if the extension has a leading . or not
    std::string theExt(x);
    auto range = std::find(theExt.begin(), theExt.end(), '.');
    theExt.erase(range);
    //Find the extension in the m_saveFileExtensionText vector
    auto vec = m_saveFileExtensionText;
    auto vecIt = std::find(vec.begin(), vec.end(), theExt);
    if (vecIt != vec.end()) {
        ptrdiff_t vecDiff = vecIt - vec.begin();
        int extLoc = static_cast<int>(vecDiff);
        return extLoc;
    }
    else {
        return notFoundVal;
    }
}//end fileExtensionIndex

//Copied from StainProfile.cpp in StainAnalysis-plugin
bool StainEvaluation::checkFile(const std::string &fileString, const std::string &op) {
    namespace fs = std::filesystem;
    const bool success = true;
    const bool errorVal = false;
    if (fileString.empty()) { return errorVal; }
    //Convert the input fileString into a filesystem::path type
    fs::path theFilePath = fs::path(fileString);
    //Check if the file exists 
    bool fileExists = fs::exists(theFilePath);

    //If op is set to "r" (read), check that the file can be opened for reading
    if (!op.compare("r") && fileExists) {
        std::ifstream inFile(fileString.c_str());
        bool result = inFile.good();
        inFile.close();
        return result;
    }
    //If op is set to "w" (write) and the file exists, check without overwriting
    else if (!op.compare("w") && fileExists) {
        //Open for appending (do not overwrite current contents)
        std::ofstream outFile(fileString.c_str(), std::ios::app);
        bool result = outFile.good();
        outFile.close();
        return result;
    }
    //If op is set to "w" (write) and the file does not exist, check if the directory exists
    else if (!op.compare("w") && !fileExists) {
        fs::path parentPath = theFilePath.parent_path();
        bool dirExists = fs::is_directory(parentPath);
        //If it does not exist, return errorVal
        if (!dirExists) { return errorVal; }
        //If it exists, does someone (anyone) have write permission?
        fs::file_status dirStatus = fs::status(parentPath);
        fs::perms dPerms = dirStatus.permissions();
        bool someoneCanWrite = ((dPerms & fs::perms::owner_write) != fs::perms::none)
            || ((dPerms & fs::perms::group_write) != fs::perms::none)
            || ((dPerms & fs::perms::others_write) != fs::perms::none);
        return dirExists && someoneCanWrite;
    }
    else {
        return errorVal;
    }
}//end checkFile

Polygon StainEvaluation::RectFToPolygon(const RectF &rectf) {
    if (isEmpty(rectf)) { return Polygon(); }
    //Get the vertices of the RectF and add them to a Polygon
    double x_min = rectf.x();
    double y_min = rectf.y();
    double x_max = sedeen::xMax(rectf);
    double y_max = sedeen::yMax(rectf);
    auto vertices = std::vector<PointF>();
    vertices.push_back(PointF(x_min, y_max));
    vertices.push_back(PointF(x_max, y_max));
    vertices.push_back(PointF(x_max, y_min));
    vertices.push_back(PointF(x_min, y_min));
    //Create new Polygon with these vertices
    Polygon newPoly = Polygon(vertices);
    return newPoly;
}//end RectFToPolygon

Polygon StainEvaluation::ChangeReferenceFrame(const Polygon &poly,
    const ImageProperties &initial, const ImageProperties &final) {
    //Build a new Polygon vertex by vertex
    std::vector<PointF> initialVertices = poly.vertices();
    std::vector<PointF> outVertices(initialVertices.size());
    //Loop through each PointF and change its reference frame
    for (auto it = initialVertices.begin(); it != initialVertices.end(); ++it) {
        int pos = static_cast<int>(it - initialVertices.begin());
        //Pass the PointF to the PointF overload of the ChangeReferenceFrame method
        //and assign the resulting point to the same position in the new vertex vector
        outVertices[pos] = ChangeReferenceFrame((*it), initial, final);
    }
    Polygon outputPolygon(outVertices);
    return outputPolygon;
}//end ChangeReferenceFrame (Polygon)

PointF StainEvaluation::ChangeReferenceFrame(const PointF &pf,
    const ImageProperties &initial, const ImageProperties &final) {
    //The coordinates of the image centers are in units of um, so pixels*pixel_size/2
    SRTTransform initialTransform = initial.sedeen_transform;
    SRTTransform finalTransform = final.sedeen_transform;
    SizeF initialPixelSize = initial.image_pixel_size;
    SizeF finalPixelSize = final.image_pixel_size;

    //Get differences between center coordinate values
    PointF centerDiff = CalculateCenterDifference(initial, final);

    //Convert the initial point coordinates to the coordinate system of the final image
    double xOutPoint = (initialPixelSize.width() * pf.getX()) / finalPixelSize.width() + centerDiff.getX();
    double yOutPoint = (initialPixelSize.height() * pf.getY()) / finalPixelSize.height() + centerDiff.getY();

    //Assign output point coordinates to the elements of a PointF
    PointF outPoint = PointF(xOutPoint, yOutPoint);
    return outPoint;
}//end ChangeReferenceFrame (PointF)







bool StainEvaluation::buildTestAndReferencePipeline() {
    using namespace image::tile;
    bool pipeline_changed = false;

    //Re-check the number of loaded images
    if (m_imageList.count() != 2) {
        return false;
    }

    bool doProcessing = false;
    if (pipeline_changed
        || m_displayArea.isChanged()
        || m_imageList.isChanged())
    {
        //The test image should be the one in the Image textbox, and 
        //so should be pointed to by image(). Get the properties of it and
        //the other loaded image (the reference image) and assign them to member variables.
        int testImageIndex = m_imageList.indexOf(image());
        int refImageIndex = 1 - m_imageList.indexOf(image());

        //Test image info, properties, and Image object pointer (workaround)
        ImageInfo testImageInfo = m_imageList.info(testImageIndex);
        std::string testImageLocation = testImageInfo.location;
        //sedeen::SRTTransform testImageTransform = testImageInfo.transform;
        auto testImageOpener = image::createImageOpener();
        m_testImage = testImageOpener->open(file::Location(testImageLocation));

        //If an image currently open in Sedeen cannot be re-opened, throw error
        if (!m_testImage) {
            throw std::runtime_error("Could not open the test image: " + file::Location(testImageLocation).getFilename());
        }

        //Temp approach to this
        //If one exists, open the session file for the image
        //The problem is that new changes to the Pixel Spacing boxes are not stored here yet
        sedeen::Session testSession = sedeen::Session(testImageLocation);
        testSession.loadFromFile();
        //
        //The pixel spacing as set in the Transform box. Stored in the Session xml file for the image
        sedeen::SizeF testTrSpacing = testSession.getPixelSize();
        //Fill the ImageProperties struct for the test image
        m_testImageProperties = GetImageProperties(m_testImage, &testImageInfo, testTrSpacing);

        //Reference image info, properties, and Image object pointer (workaround)
        ImageInfo refImageInfo = m_imageList.info(refImageIndex);
        std::string refImageLocation = refImageInfo.location;
        sedeen::SRTTransform refImageTransform = refImageInfo.transform;
        auto refImageOpener = image::createImageOpener();
        m_refImage = refImageOpener->open(file::Location(refImageLocation));
        //If an image currently open in Sedeen cannot be re-opened, throw error
        if (!m_refImage) {
            throw std::runtime_error("Could not open the reference image: " + file::Location(refImageLocation).getFilename());
        }
        //Temp approach to this
        //If one exists, open the session file for the image
        //The problem is that new changes to the Pixel Spacing boxes are not stored here yet
        sedeen::Session refSession = sedeen::Session(refImageLocation);
        refSession.loadFromFile();
        //
        //The pixel spacing as set in the Transform box. Stored in the Session xml file for the image
        sedeen::SizeF refTrSpacing = refSession.getPixelSize();
        //Fill the ImageProperties struct for the reference image
        m_refImageProperties = GetImageProperties(m_refImage, &refImageInfo, refTrSpacing);




        //Threshold
        //Crop
        //Resample
        //Mask
        //Pixel-to-pixel operations






        //Try cropping



        //auto display_resolution = getDisplayResolution(image(), m_displayArea);

        //Get the Behavior value from the m_retainment
        //Scale down the threshold to create more precision
        //auto threshold_kernel =
        //    std::make_shared<image::tile::ODThresholdKernel>(m_threshold / 100.0,
        //    behaviorVal, theWeights);

        // Create a Factory for the composition of these Kernels
        //auto non_cached_factory =
        //    std::make_shared<FilterFactory>(source_factory, threshold_kernel);

        // Wrap resulting Factory in a Cache for speedy results
        //m_ODThreshold_factory =
        //    std::make_shared<Cache>(non_cached_factory, RecentCachePolicy(30));

        //pipeline_changed = true;


    }//end if parameter values changed

    //
    // Constrain processing to the region of interest provided, if set
    //std::shared_ptr<GraphicItemBase> region = m_regionToProcess;
    //if (pipeline_changed && (nullptr != region)) {
    //    // Constrain the output of the pipeline to the region of interest provided
    //    auto constrained_factory = std::make_shared<RegionFactory>(m_ODThreshold_factory, region->graphic());

    //    // Wrap resulting Factory in a Cache for speedy results
    //    m_ODThreshold_factory = std::make_shared<Cache>(constrained_factory, RecentCachePolicy(30));
    //}

    return pipeline_changed;
}//end buildPipeline



























} // namespace algorithm
} // namespace sedeen
