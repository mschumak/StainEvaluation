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

#ifndef SEDEEN_SRC_PLUGINS_STAINEVALUATION_STAINEVALUATION_H
#define SEDEEN_SRC_PLUGINS_STAINEVALUATION_STAINEVALUATION_H

//Sedeen required headers
#include <algorithm/AlgorithmBase.h>
#include <algorithm/Parameters.h>
#include <algorithm/Results.h>
#include <algorithm/ImageListParameter.h>
#include <image/io/DataServer.h>

#include <global/ColorSpace.h>
#include <global/geometry/SRTTransform.h>
#include <global/geometry/SizeF.h>
#include <archive/Session.h>

namespace sedeen {
namespace tile {

} // namespace tile

namespace algorithm {
///struct to hold the image properties
struct ImageProperties
{
    std::basic_string <char> location;
    sedeen::SRTTransform sedeen_transform;
    ///Pixel spacing set in the Sedeen Transform box, stored in session file
    sedeen::SizeF tr_pixel_spacing; //um units
    //The color model and channel/pixel type can be obtained from an image's ColorSpace
    sedeen::ColorModel color_model;
    sedeen::ChannelType pixel_type;
        
    int opacity;
    bool visibility;

    int nlevels;
    ///Pixel size as read from the image
    sedeen::SizeF  image_pixel_size;
    sedeen::Size   image_size;
};

///StainEvaluation plugin for Sedeen Viewer
class StainEvaluation : public algorithm::AlgorithmBase {
public:
	// Constructor
    StainEvaluation();
	// Destructor
	virtual ~StainEvaluation();


private:
	//Private methods
	virtual void init(const image::ImageHandle& image);
    virtual void run();


    ///The actual comparison this plugin is intended to accomplish
    bool buildTestAndReferencePipeline();

    ///Create a reference image by selecting pixels from an image using a pixel mask
    bool buildApplyMaskPipeline();


    

    image::RawImage GetDisplayImage(const image::ImageHandle& im);





    ImageProperties GetImageProperties(const image::ImageHandle& im, sedeen::algorithm::ImageInfo *imageinfo, 
        const sedeen::SizeF &trSpacing = sedeen::SizeF(1.0,1.0));


    bool SetImageInfo(int index);

    ///Create text for a report containing the properties of 
    const std::string generateImagePropertiesReport(const ImageProperties &ip);


private:
    DisplayAreaParameter m_displayArea;
    ImageListParameter m_imageList;

    //GraphicItemParameter m_regionToProcess; //single output region


    double m_thresholdParameter; //change this to a DoubleParameter later


    /// User defined Threshold value.
    //algorithm::DoubleParameter m_threshold;

    /// The output result
    ImageResult m_result;
    TextResult m_outputText;
    std::string m_report;




    //Split this out into another plugin when I understand what I'm doing

    ///The mask image pointer (e.g. DAPI)
    std::shared_ptr<sedeen::image::Image> m_maskImage;
    ///The source image pointer (to apply the mask to)
    std::shared_ptr<sedeen::image::Image> m_sourceImage;

    ///Properties of the mask image, obtained from the Image and ImageInfo objects
    ImageProperties m_maskImageProperties;
    ///Properties of the source image, obtained from the Image and ImageInfo objects
    ImageProperties m_sourceImageProperties;





    ///The test image pointer (modified image)
    std::shared_ptr<sedeen::image::Image> m_testImage;
    ///The reference image pointer (to compare test images against)
    std::shared_ptr<sedeen::image::Image> m_refImage;

    ///Properties of the test image, obtained from the Image and ImageInfo objects
    ImageProperties m_testImageProperties;
    ///Properties of the reference image, obtained from the Image and ImageInfo objects
    ImageProperties m_refImageProperties;




    /// The intermediate image factory after thresholding
    //std::shared_ptr<image::tile::Factory> m_ODThreshold_factory;

private:
    //Member variables

};

} // namespace algorithm
} // namespace sedeen

#endif
