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

namespace sedeen {
namespace tile {

} // namespace tile

namespace algorithm {

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

    /// Creates the thresholding pipeline with a cache
    //
    /// \return 
    /// TRUE if the pipeline has changed since the call to this function, FALSE
    /// otherwise
    bool buildPipeline();

private:
    DisplayAreaParameter m_displayArea;

    GraphicItemParameter m_regionToProcess; //single output region

    /// User defined Threshold value.
    //algorithm::DoubleParameter m_threshold;

    /// The output result
    ImageResult m_result;
    TextResult m_outputText;
    std::string m_report;

    /// The intermediate image factory after thresholding
    //std::shared_ptr<image::tile::Factory> m_ODThreshold_factory;

private:
    //Member variables

};

} // namespace algorithm
} // namespace sedeen

#endif
