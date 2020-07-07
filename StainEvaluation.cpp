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

//Sedeen required headers
#include "Algorithm.h"
#include "Geometry.h"
#include "Global.h"
#include "Image.h"
#include "image/io/Image.h"
#include "image/tile/Factory.h"

// Poco header needed for the macros below 
#include <Poco/ClassLibrary.h>


#include <sstream>


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
    //m_regionToProcess(),

    m_thresholdParameter(10.0), //temp

    //Member data

    m_maskImage(),
    m_sourceImage(),
    m_maskImageProperties(),
    m_sourceImageProperties(),

    m_testImage(),
    m_refImage(),
    m_testImageProperties(),
    m_refImageProperties(),
    //Output objects
    m_result(),
    m_outputText(),
    m_report("")
{
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


    //an option parameter for the type of error measurement?
    //or just do them all?



    //Assemble the user interface
    // Bind result
    m_outputText = createTextResult(*this, "Text Result");
    m_result = createImageResult(*this, " StainEvaluationResult");

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
    bool imageChecksPassed = true;
    if (m_imageList.count() != 2) {
        report.append("This plugin requires exactly two images to be loaded. ");


        report.append("Please load a source image and a mask image in Sedeen. ");
        report.append("Click on the MASK image to highlight it. ");
        report.append("Check that the MASK image location is in the Image text box at the top of the Analysis Manager.");


        //These will be for the comparison!
        //report.append("Please load a test image and a reference image in Sedeen. ");
        //report.append("Click on the test image to highlight it. ");
        //report.append("Check that the test image location is in the Image text box at the top of the Analysis Manager.");
        
        
        imageChecksPassed = false;
    }
    //additional image checks?

    //check whether the images intersect, provide helpful error message if not


    //Keep this condition separate so that additional image checks can be added
    bool pipeline_changed = false;
    if (imageChecksPassed) {
        report.append("Entered the clause to build the pipeline\n");


        //This is for applying the mask image to a source (DAPI on unseparated image)
        pipeline_changed = buildApplyMaskPipeline();









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
    }
    else {
        //Send the report immediately
        m_outputText.sendText(report);
    }

    if (imageChecksPassed && 
        (display_changed 
        || image_list_changed 
        || pipeline_changed)  ) {

        //m_result.update(m_ODThreshold_factory, m_displayArea, *this);


        // Update the output text report
        if (false == askedToStop()) {
            //auto report = generateCompleteReport();
            m_outputText.sendText(report);

            // Get image from the output factory
            //auto compositor = std::make_unique<image::tile::Compositor>(m_ODThreshold_factory);

            //DisplayRegion region = m_displayArea;
            //auto output_image = compositor->getImage(region.source_region, region.output_size);

            // Get image from the input factory
            //auto compositorsource = std::make_unique<image::tile::Compositor>(image()->getFactory());
            //auto input_image = compositorsource->getImage(region.source_region, region.output_size);

            //if (m_regionToProcess.isUserDefined()) {
            //     std::shared_ptr<GraphicItemBase> roi = m_regionToProcess;
            //    auto display_resolution = getDisplayResolution(image(), m_displayArea);
            //    Rect rect = containingRect(roi->graphic());
            //    output_image = compositor->getImage(rect, region.output_size);
            //}
        }
    }//if display or pipeline changed

    // Ensure we run again after an abort
    if (askedToStop()) {
        //m_ODThreshold_factory.reset();

        //reset the factory, whatever it ends up being
    }
}//end run



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
        || m_imageList.isChanged()   )
    {

        //m_maskImage;
        //m_sourceImage;

        //m_maskImageProperties;
        //m_sourceImageProperties;


        //The MASK image should be the one in the Image textbox, and 
        //so should be pointed to by image(). Get the properties of it and
        //the other loaded image (the source image) and assign them to member variables.

        int maskImageIndex = m_imageList.indexOf(image());
        int sourceImageIndex = 1 - m_imageList.indexOf(image());

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

        //Temp approach to this
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
        //Temp approach to this
        //If one exists, open the session file for the image
        //The problem is that new changes to the Pixel Spacing boxes are not stored here yet
        sedeen::Session sourceSession = sedeen::Session(sourceImageLocation);
        sourceSession.loadFromFile();
        //
        //The pixel spacing as set in the Transform box. Stored in the Session xml file for the image
        sedeen::SizeF sourceTrSpacing = sourceSession.getPixelSize();
        //Fill the ImageProperties struct for the source image
        m_sourceImageProperties = GetImageProperties(m_sourceImage, &sourceImageInfo, sourceTrSpacing);







        //Get the containing rectangles of the images
        sedeen::Rect maskContainingRect = sedeen::image::rect(m_maskImage, 0);
        sedeen::Rect sourceContainingRect = sedeen::image::rect(m_sourceImage, 0);
        //Find the intersection of the two rectangles
        sedeen::Rect intersectionRect = sedeen::intersection(maskContainingRect, sourceContainingRect);
        //If the intersection is empty, set doProcessing to false
        bool intersectionEmpty = sedeen::isEmpty(intersectionRect);
        if (intersectionEmpty) {
            pipeline_changed = false;
            //temp: will handle the return differently later
            return pipeline_changed;
        }
        else {


            //Crop the source image to the intersection rectangle











        }



        //Threshold
        //Crop
        //Resample
        //Mask
        //Save both cropped

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

    double x_pixel_size = 1;	//default val, mm
    double y_pixel_size = 1;
    if (im->getMetaData()->has(image::DoubleTags::PIXEL_SIZE_X)) {
        x_pixel_size = im->getMetaData()->get(image::DoubleTags::PIXEL_SIZE_X, 0) / 1000; //mm
    }
    if (im->getMetaData()->has(image::DoubleTags::PIXEL_SIZE_Y)) {
        y_pixel_size = im->getMetaData()->get(image::DoubleTags::PIXEL_SIZE_Y, 0) / 1000;
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
    ss << "    Pixel Width: " << image_pixel_size.width() << std::endl;
    ss << "    Pixel Height: " << image_pixel_size.height() << std::endl;
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




image::RawImage StainEvaluation::GetDisplayImage(const image::ImageHandle& im)
{
    DisplayRegion display_region = m_displayArea;
    // Get the region currently on screen

    auto displayRegionScale = getDisplayScale(display_region);
    auto imDisplayIterator = getIterator(im, display_region);
    Point displayRegionPosition(imDisplayIterator.x(), imDisplayIterator.y());
    auto imageDisplaySize = display_region.output_size;
    Rect imageDisplayRect(displayRegionPosition, imageDisplaySize);

    image::tile::Compositor compositor(im->getFactory());
    auto input = compositor.getImage(imageDisplayRect, display_region.output_size);
    return input;
}//end GetDisplayImage




bool StainEvaluation::SetImageInfo(int index)
{
    //auto imageinfo = m_imageList.info(index);
    //imageinfo.transform.setCenter(s_image_props_.centre);
    //imageinfo.transform.setRotation(sedeen_transform_.rotation());
    //imageinfo.transform.setTranslation(sedeen_transform_.translation());
    //imageinfo.transform.setScale(sedeen_transform_.scale());
    //imageinfo.transform.setCenter(sedeen_transform_.center());
    //m_imageList.setInfo(index, imageinfo);

    return true;
}











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
