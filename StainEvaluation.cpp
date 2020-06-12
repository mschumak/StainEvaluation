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
    m_regionToProcess(),

    //Member data
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
        report.append("Please load a test image and a reference image in Sedeen. ");
        report.append("Click on the test image to highlight it. ");
        report.append("Check that the test image location is in the Image text box at the top of the Analysis Manager.");
        imageChecksPassed = false;
    }
    //additional image checks?

    //Keep this condition separate so that additional image checks can be added
    bool pipeline_changed = false;
    if (imageChecksPassed) {
        report.append("Entered the clause to build the pipeline\n");

        pipeline_changed = buildPipeline();

        //Build a report 

        //int testImageIndex = m_imageList.indexOf(image());
        //int refImageIndex = 1 - m_imageList.indexOf(image());
        //std::stringstream ss;
        //ss << "The test image index is: " << testImageIndex;
        //ss << " and the reference image index is: " << refImageIndex << std::endl;
        //report.append(ss.str());


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



bool StainEvaluation::buildPipeline() {
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
        m_testImageProperties = GetImageProperties(m_testImage, &testImageInfo);

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
        m_refImageProperties = GetImageProperties(m_refImage, &refImageInfo);





    // Get source image properties
    //auto source_factory = image()->getFactory();
    //auto source_color = source_factory->getColorSpace();
































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



ImageProperties StainEvaluation::GetImageProperties(const image::ImageHandle& im, sedeen::algorithm::ImageInfo *imageinfo)
{
    ImageProperties imProps;

    //auto imageinfo = image_list_.info(index);     //image_list_.indexOf(image()));
    imProps.sedeen_transform = imageinfo->transform;
    imProps.opacity = imageinfo->opacity;
    imProps.visibility = imageinfo->visible;
    imProps.location = imageinfo->location;

    //auto dataServer = createDataServer(im);
    //imProps.nlevels = dataServer->getNumLevels();


    int im_width = im->getMetaData()->get(image::IntegerTags::IMAGE_X_DIMENSION, 0);  // pixel
    int im_height = im->getMetaData()->get(image::IntegerTags::IMAGE_Y_DIMENSION, 0);
    imProps.size = sedeen::Size(im_width, im_height);

    double x_spacing = 1;	//mm with no pixel spacing
    double y_spacing = 1;
    if (im->getMetaData()->has(image::DoubleTags::PIXEL_SIZE_X))
        x_spacing = im->getMetaData()->get(image::DoubleTags::PIXEL_SIZE_X, 0) / 1000; //mm
    if (im->getMetaData()->has(image::DoubleTags::PIXEL_SIZE_Y))
        y_spacing = im->getMetaData()->get(image::DoubleTags::PIXEL_SIZE_Y, 0) / 1000;
    imProps.spacing = sedeen::SizeF(x_spacing, y_spacing);

    double x_centre = im->getMetaData()->get(image::DoubleTags::IMAGE_CENTRE_X, 0) / 2;   //IMAGE_CENTRE_X is in mm
    double y_centre = im->getMetaData()->get(image::DoubleTags::IMAGE_CENTRE_Y, 0) / 2;	  //IMAGE_CENTRE_Y is in mm
    imProps.centre = sedeen::PointF(x_centre, y_centre);

    return imProps;
}//end GetImageProperties



std::string StainEvaluation::generateImagePropertiesReport(ImageProperties ip) {


    return std::string();
}//end generateImagePropertiesReport











image::RawImage StainEvaluation::GetRawImage(const image::ImageHandle& im)
{
    auto dataServer = createDataServer(im);
    auto nlevels = dataServer->getNumLevels();
    auto lowResRect = rect(im, nlevels - 1);
    auto rawImage = dataServer->getImage(nlevels - 1, lowResRect);
    if (rawImage.isNull())
        throw("Could not open the target image");

    return rawImage;
}//end GetRawImage




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



} // namespace algorithm
} // namespace sedeen
