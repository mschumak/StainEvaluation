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
#include "algorithm/AlgorithmBase.h"
#include "algorithm/Parameters.h"
#include "algorithm/Results.h"
#include "algorithm/ImageListParameter.h"	
#include "image/io/DataServer.h"

#include <global/geometry/SRTTransform.h>
#include <global/geometry/SizeF.h>

namespace sedeen {
namespace tile {

} // namespace tile

namespace algorithm {
    ///This struct holds the image properties
    struct ImageProperties
    {
        std::basic_string <char> location;
        int nlevels;
        sedeen::SRTTransform sedeen_transform;
        int opacity;
        bool visibility;
        sedeen::SizeF  spacing;
        sedeen::Size   size;
        sedeen::PointF centre;

        double x_centre;
        double y_centre;
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

    bool buildPipeline();


    image::RawImage GetRawImage(const image::ImageHandle& im);
    image::RawImage GetDisplayImage(const image::ImageHandle& im);





    ImageProperties GetImageProperties(const image::ImageHandle& im, sedeen::algorithm::ImageInfo *imageinfo);


    bool SetImageInfo(int index);

    ///Create text for a report containing the properties of 
    std::string generateImagePropertiesReport(ImageProperties ip);


private:
    DisplayAreaParameter m_displayArea;
    ImageListParameter m_imageList;

    GraphicItemParameter m_regionToProcess; //single output region



    /// User defined Threshold value.
    //algorithm::DoubleParameter m_threshold;

    /// The output result
    ImageResult m_result;
    TextResult m_outputText;
    std::string m_report;

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
