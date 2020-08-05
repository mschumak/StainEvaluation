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
#include <global/geometry/RectF.h>
#include <archive/Session.h>

namespace sedeen {
namespace tile {

} // namespace tile

namespace algorithm {
///struct to hold properties of an image obtained from multiple sources of information
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
    sedeen::SizeF  image_pixel_size; //um units
    sedeen::Size   image_size;

    //Unit conversion (between millimeters and micrometers)
    double um_per_mm = 1000.0;
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


    ///The actual comparison this plugin is intended to accomplish (next)
    bool buildTestAndReferencePipeline();

    ///Create a reference image by selecting pixels from an image using a pixel mask
    bool buildApplyMaskPipeline();


    

    //image::RawImage GetDisplayImage(const image::ImageHandle& im);

    ///Collect image information from the Image, Sedeen's ImageInfo, and additional pixel spacing information
    ImageProperties GetImageProperties(const image::ImageHandle& im, sedeen::algorithm::ImageInfo *imageinfo, 
        const sedeen::SizeF &trSpacing = sedeen::SizeF(1.0,1.0));


    ///Define the save file dialog options outside of init
    sedeen::file::FileDialogOptions defineSaveFileDialogOptions(const std::string &capt = std::string());

    ///Get the expected number of pixels to be saved in an output file cropped to the given Rect.
    double EstimateOutputImageSize(const sedeen::Rect &r);
    ///Get a human-readable estimate of the storage space required for an output file (with 4 bytes per pixel).
    std::string EstimateImageStorageSize(const double &pix);

    ///Check whether a given full file path can be written to and has the right extension; sends report with member variable
    bool checkImageSaveProperties(const std::string &path, const std::string &rep, const std::string &desc);

    ///Save the image with a given Factory within a given Rect to a TIF/PNG/BMP/GIF/JPG flat format file
    bool SaveCroppedImageToFile(std::shared_ptr<image::tile::Factory> factory, const std::string &p, const sedeen::Rect &rect);

    ///Given a full file path as a string, identify if there is an extension and return it
    const std::string getExtension(const std::string &p);

    ///Search the m_saveFileExtensionText vector for a given extension, and return the index, or -1 if not found
    const int findExtensionIndex(const std::string &x) const;

    ///Check if the file exists and accessible for reading or writing, or that the directory to write to exists; copied from StainProfile
    bool checkFile(const std::string &fileString, const std::string &op);

    ///Use the corners of a RectF object to define a 4-vertex Polygon
    Polygon RectFToPolygon(const RectF &rectf);
    ///Transform a Polygon when there are transforms applied to both the initial and final image spaces
    Polygon TransformPolygon(const Polygon &poly, const ImageProperties &initial, const ImageProperties &final);

    ///Change the frame of reference of a Polygon from an initial image to a final image space
    Polygon ChangeReferenceFrame(const Polygon &poly, const ImageProperties &initial, const ImageProperties &final);
    ///Change the frame of reference of a PointF from an initial image to a final image space
    PointF ChangeReferenceFrame(const PointF &pf, const ImageProperties &initial, const ImageProperties &final);

    ///Gets center() from transform in ImageProperties, or if it is (0,0), returns half the image size multiplied by the pixel size.
    PointF GetImageCenterFromProperties(const ImageProperties &ip);

    ///Calculate the change in location of the image center of the initial image in the final image reference frame
    PointF CalculateCenterDifference(const ImageProperties &initial, const ImageProperties &final);

    ///Create text for a report containing the properties of 
    const std::string generateImagePropertiesReport(const ImageProperties &ip);


private:
    DisplayAreaParameter m_displayArea;
    ImageListParameter m_imageList;

    /// User defined threshold to apply to the mask image
    algorithm::DoubleParameter m_maskThreshold;

    ///User choice of file name for the image cropped to the intersection of source and mask
    SaveFileDialogParameter m_saveCroppedImageFileAs;
    ///User choice of file name for the image with the mask image applied
    SaveFileDialogParameter m_saveMaskedImageFileAs;

    /// The outputs
    ImageResult m_result;
    TextResult m_outputText;
    OverlayResult m_overlayResult;
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

    ///Store the rectangle that is the intersection of the mask and source outer rectangles
    sedeen::Rect m_maskSourceIntersectionRect;
 
    ///Apply a crop and pixel mask to the source image
    std::shared_ptr<image::tile::Factory> m_Mask_factory;


    ///The test image pointer (modified image)
    std::shared_ptr<sedeen::image::Image> m_testImage;
    ///The reference image pointer (to compare test images against)
    std::shared_ptr<sedeen::image::Image> m_refImage;

    ///Properties of the test image, obtained from the Image and ImageInfo objects
    ImageProperties m_testImageProperties;
    ///Properties of the reference image, obtained from the Image and ImageInfo objects
    ImageProperties m_refImageProperties;


private:
    //Member variables


    std::vector<std::string> m_saveFileExtensionText;
    double m_maskThresholdDefaultVal;
    double m_maskThresholdMaxVal;
    ///Number of pixels in an image to be saved over which the user will receive a warning.
    double m_pixelWarningThreshold;




};

} // namespace algorithm
} // namespace sedeen

#endif
