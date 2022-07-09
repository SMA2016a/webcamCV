/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   imageprocessor.h
 * Author: root
 *
 * Created on July 1, 2022, 4:02 PM
 */

#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <boost/filesystem.hpp>
#include <pylon/PylonIncludes.h>
#include <boost/thread.hpp>
// The sample uses the std::list.
#include <list>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <filesystem>

// Extend the pylon API for using pylon data processing.
#include <pylondataprocessing/PylonDataProcessingIncludes.h>
using namespace Pylon;
using namespace std;
using namespace Pylon::DataProcessing;

namespace fs = boost::filesystem;
using namespace boost;


// Declare a data class for one set of output data values.
class ResultData
{
public:
    ResultData()
        : hasError(false)
    {
    }

    CPylonImage image; // The image from the camera

    StringList_t decodedBarcodes; // The decoded barcodes

    bool hasError;     // If something doesn't work as expected
                       // while processing data, this is set to true.

    String_t errorMessage; // Contains an error message if
                           // hasError has been set to true.
};

// MyOutputObserver is a helper object that shows how to handle output data
// provided via the IOutputObserver::OutputDataPush interface method.
// Also, MyOutputObserver shows how a thread-safe queue can be implemented
// for later processing while pulling the output data.
class MyOutputObserver : public IOutputObserver
{
public:
    MyOutputObserver()
        : m_waitObject(WaitObjectEx::Create())
    {
    }

    // Implements IOutputObserver::OutputDataPush.
    // This method is called when an output of the CRecipe pushes data out.
    // The call of the method can be performed by any thread of the thread pool of the recipe.
    void OutputDataPush(
        CRecipe& recipe,
        CVariantContainer valueContainer,
        const CUpdate& update,
        intptr_t userProvidedId) override
    {
        // The following variables are not used here:
        PYLON_UNUSED(recipe);
        PYLON_UNUSED(update);
        PYLON_UNUSED(userProvidedId);

        ResultData currentResultData;

        // Get the result data of the recipe via the output terminal's "Image" pin.
        // The value container is a dictionary/map-like type.
        // Look for the key in the dictionary.
        auto posImage = valueContainer.find("Image");
        // We expect there to be an image
        // because the recipe is set up to behave like this.
        PYLON_ASSERT(posImage != valueContainer.end());
        if (posImage != valueContainer.end())
        {
            // Now we can use the value of the key/value pair.
            const CVariant& value = posImage->second;
            if (!value.HasError())
            {
                currentResultData.image = value.ToImage();
            }
            else
            {
                currentResultData.hasError = true;
                currentResultData.errorMessage = value.GetErrorDescription();
            }
        }

        // Get the data from the Barcodes pin.
        auto posBarcodes = valueContainer.find("Texts");
       

        if (posBarcodes != valueContainer.end())
        {
            const CVariant& value = posBarcodes->second;
            if (!value.HasError())
            {
                for(size_t i = 0; i < value.GetNumArrayValues(); ++i)
                {
                    const CVariant decodedBarcodeValue = value.GetArrayValue(i);
                    if (!decodedBarcodeValue.HasError())
                    {
                        currentResultData.decodedBarcodes.push_back(decodedBarcodeValue.ToString());
                         //cout <<"current code : " << decodedBarcodeValue.ToString();
                    }
                    else
                    {
                        currentResultData.hasError = true;
                        currentResultData.errorMessage = value.GetErrorDescription();
                        break;
                    }
                }
            }
            else
            {
                currentResultData.hasError = true;
                currentResultData.errorMessage = value.GetErrorDescription();
            }
            
            
        }
        
        auto posRegions = valueContainer.find("Centers_px");
        if (posRegions != valueContainer.end())
        {
           const CVariant& value = posRegions->second;
            if (!value.HasError())
            {
                int count = value.GetNumArrayValues();
                
                for(size_t i = 0; i < value.GetNumArrayValues(); ++i)
                {
                    const CVariant decodedRegionValue = value.GetArrayValue(i);
                    if (!decodedRegionValue.HasError())
                    {
                       // currentResultData.po.push_back(decodedRegionValue.ToString());
                         cout <<"current pos : " << decodedRegionValue.ToString() <<endl;
                    }
                    else
                    {
                        currentResultData.hasError = true;
                        currentResultData.errorMessage = value.GetErrorDescription();
                        break;
                    }
                }
            }
            else
            {
                currentResultData.hasError = true;
                currentResultData.errorMessage = value.GetErrorDescription();
            }
        }
        
        
        

        // Add data to the result queue in a thread-safe way.
        {
            AutoLock scopedLock(m_memberLock);
            m_queue.emplace_back(currentResultData);
        }

        // Signal that data is ready.
        m_waitObject.Signal();
    }

    // Get the wait object for waiting for data.
    const WaitObject& GetWaitObject()
    {
        return m_waitObject;
    }

    // Get one result data object from the queue.
    bool GetResultData(ResultData& resultDataOut)
    {
        AutoLock scopedLock(m_memberLock);
        if (m_queue.empty())
        {
            return false;
        }

        resultDataOut = std::move(m_queue.front());
        m_queue.pop_front();
        if (m_queue.empty())
        {
            m_waitObject.Reset();
        }
        return true;
    }

private:
    CLock m_memberLock;        // The member lock is required to ensure
                               // thread-safe access to the member variables.
    WaitObjectEx m_waitObject; // Signals that ResultData is available.
                               // It is set if m_queue is not empty.
    list<ResultData> m_queue;  // The queue of ResultData
};


class imageprocessor {
public:
    imageprocessor();
    imageprocessor(const imageprocessor& orig);
    virtual ~imageprocessor();
    int doBarcodeeading();
    int doBarcodeeadingGR(list<CPylonImage>);
    int doBarcodeeadingImages(std::string);
    int doBarcodeeadingCV(list<CPylonImage>);

    std::thread startAsync(const char*, unsigned);
    bool stop();
    void appendGrabResult(CGrabResultPtr);
private:
    CImageFormatConverter m_fc;
    CPylonImage image;
    bool done = false;
    std::thread  worker;
    vector<CGrabResultPtr>m_grabResults;
    std::mutex m_mutx;

};

#endif /* IMAGEPROCESSOR_H */

