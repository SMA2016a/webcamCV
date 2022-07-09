/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   imageprocessor.cpp
 * Author: root
 *
 * Created on July 1, 2022, 4:02 PM
 */

#include "imageprocessor.h"

imageprocessor::imageprocessor()
{
}

imageprocessor::imageprocessor(const imageprocessor &orig)
{
}

imageprocessor::~imageprocessor()
{
}

void imageprocessor::appendGrabResult(CGrabResultPtr gr)
{
    m_mutx.lock();
    m_grabResults.push_back(gr);
    m_mutx.unlock();
}

int imageprocessor::doBarcodeeadingImages(std::string path)
{
    vector<string> ImagefileNames;
    try
    {
        std::string path = "/wp/qrcode";
        for (const auto &entry : fs::directory_iterator(path))
        {
            ImagefileNames.push_back(entry.path().c_str());
            std::cout << entry.path() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }

    try
    {
        // This object is used for collecting the output data.
        // If placed on the stack it must be created before the recipe
        // so that it is destroyed after the recipe.
        MyOutputObserver resultCollector;

        // Create a recipe representing a design created using
        // the pylon Viewer Workbench.
        CRecipe recipe;

        // Load the recipe file.
        // Note: PYLON_DATAPROCESSING_CAMERA_RECIPE is a string
        // created by the CMake build files.
        // recipe.Load("/workspace/recipe/barcode.precipe");
        // recipe.Load("/workspace/recipe/QR.precipe");
        recipe.Load("/wp/recipe/RecipeGR.precipe");
        // recipe.PreAllocateResources();

        // recipe.GetParameters().Get(StringParameterName("Camera/@CameraDevice/ImageFilename")).SetValue("/workspace/qrcode");
        //  For demonstration purposes only
        //  Let's check the Pylon::CDeviceInfo properties of the camera we are going to use.
        //  Basler recommends using the DeviceClass and the UserDefinedName to identify a camera.
        //  The UserDefinedName is taken from the DeviceUserID parameter that you can set in the pylon Viewer's Features pane.
        //  Note: USB cameras must be disconnected and reconnected or reset to provide the new DeviceUserID.
        //  This is due to restrictions defined by the USB standard.
        cout << "Properties used for selecting a camera device" << endl;
        CIntegerParameter devicePropertySelector =
            recipe.GetParameters().Get(IntegerParameterName("Camera/@vTool/DevicePropertySelector"));
        if (devicePropertySelector.IsWritable())
        {
            CStringParameter deviceKey = recipe.GetParameters().Get(StringParameterName("Camera/@vTool/DevicePropertyKey"));
            CStringParameter deviceValue = recipe.GetParameters().Get(StringParameterName("Camera/@vTool/DevicePropertyValue"));
            for (int64_t i = devicePropertySelector.GetMin(); i <= devicePropertySelector.GetMax(); ++i)
            {
                devicePropertySelector.SetValue(i);
                cout << deviceKey.GetValue() << "=" << deviceValue.GetValue() << endl;
            }
        }
        else
        {
            cout << "The first camera device found is used." << endl;
        }

        // For demonstration purposes only
        // Print available parameters.
        //        {
        //            cout << "Parameter names before allocating resources" << endl;
        //            StringList_t parameterNames = recipe.GetParameters().GetAllParameterNames();
        //            for (const auto& name : parameterNames)
        //            {
        //                cout << name << endl;
        //            }
        //        }

        // Allocate the required resources. This includes the camera device.
        recipe.PreAllocateResources();
        // recipe.GetParameters().Get(StringParameterName("Camera/@CameraDevice/ImageFilename")).SetValue("/workspace/recipe/Emulation_0815-0000.pfs");

        // For demonstration purposes only
        cout << "Selected camera device:" << endl;
        cout << "ModelName=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceModelName")).GetValueOrDefault("N/A") << std::endl;
        cout << "SerialNumber=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceSerialNumber")).GetValueOrDefault("N/A") << std::endl;
        cout << "VendorName=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceVendorName")).GetValueOrDefault("N/A") << std::endl;
        cout << "UserDefinedName=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceUserDefinedName")).GetValueOrDefault("N/A") << std::endl;
        // StringParameterName is the type of the parameter.
        // MyCamera is the name of the vTool.
        // Use @vTool if you want to access the vTool parameters.
        // Use @CameraInstance if you want to access the parameters of the CInstantCamera object used internally.
        // Use @DeviceTransportLayer if you want to access the transport layer parameters.
        // Use @CameraDevice if you want to access the camera device parameters.
        // Use @StreamGrabber0 if you want to access the camera device parameters.
        // SelectedDeviceUserDefinedName is the name of the parameter.

        // For demonstration purposes only
        // Print available parameters after allocating resources. Now we can access the camera parameters.
        //        {
        //            cout << "Parameter names after allocating resources" << endl;
        //            StringList_t parameterNames = recipe.GetParameters().GetAllParameterNames();
        //            for (const auto& name : parameterNames)
        //            {
        //                cout << name << endl;
        //            }
        //        }

        // For demonstration purposes only
        // Print available output names.
        StringList_t outputNames;
        recipe.GetOutputNames(outputNames);
        for (const auto &outputName : outputNames)
        {
            cout << "Output found: " << outputName << std::endl;
        }

        // Register the helper object for receiving all output data.
        recipe.RegisterAllOutputsObserver(&resultCollector, RegistrationMode_Append);

        // Start the processing. The recipe is triggered internally
        // by the camera vTool for each image.
        recipe.Start();
        CEnumParameter testImageSelector =
            recipe.GetParameters().Get(EnumParameterName("Camera/@CameraDevice/TestImageSelector"));

        for(auto const val : images)
        {
            CGrabResultPtr gr;
            m_mutx.lock();
            if (m_grabResults.size() > 0)
            {
                gr = m_grabResults[0];
                m_grabResults.erase(m_grabResults.begin());
            }
            m_mutx.unlock();

            if (gr->GrabSucceeded())
            {
                CPylonImage srcImage;
                srcImage.AttachGrabResultBuffer(gr);

                CVariant srcVariant(srcImage);
                recipe.TriggerUpdateAsync("GRImageImput", srcVariant, nullptr, gr->GetBlockID());

                if (resultCollector.GetWaitObject().Wait(5000))
                {
                    ResultData result;
                    resultCollector.GetResultData(result);
                    if (!result.hasError)
                    {
                        // Access the image data.
                        cout << "SizeX: " << result.image.GetWidth() << endl;
                        cout << "SizeY: " << result.image.GetHeight() << endl;
                        cout << "image count " << gr->GetBlockID() << endl;
                        // const uint8_t* pImageBuffer = (uint8_t*)result.image.GetBuffer();
                        // cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;
                        int i = 1;
                        for (auto val : result.decodedBarcodes)
                        {

                            cout << "found text : " << val << endl;
                        }

                        // m_fc.OutputPixelFormat = PixelType_RGB8packed;
                        // m_fc.Convert(image, result.image);

                        // Mat cv_img(result.image.GetHeight(), result.image.GetWidth(), CV_8U, (uint8_t *)result.image.GetBuffer());
                        // Mat cv_img_small;
                        // cv::resize(cv_img, cv_img_small, cv::Size(), 0.5, 0.5);
                        // imshow(windowsName, cv_img_small);
                        //  if ((waitKey(5) & 0xEFFFFF) == 27)
                        //  {
                        //      done = true;
                        //  }
                    }
                    else
                    {
                        cout << "An error occurred in processing: " << result.errorMessage << endl;
                    }
                }
                else
                {
                    throw RUNTIME_EXCEPTION("Result timeout");
                }
            }
            else
            {
                cout << "no Grabresult aviable " << endl;
            }
        }
        destroyAllWindows();
        // Stop the processing.
        recipe.Stop();

        // Optionally, deallocate resources.
        recipe.DeallocateResources();
    }
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
             << e.GetDescription() << endl;
        exitCode = 1;
    }
    return exitCode;
}
int imageprocessor::doBarcodeeading()
{
    using namespace cv;
    int exitCode = 0;
    const char *windowsName = "Result";

    try
    {
        // This object is used for collecting the output data.
        // If placed on the stack it must be created before the recipe
        // so that it is destroyed after the recipe.
        MyOutputObserver resultCollector;

        // Create a recipe representing a design created using
        // the pylon Viewer Workbench.
        CRecipe recipe;

        // Load the recipe file.
        // Note: PYLON_DATAPROCESSING_CAMERA_RECIPE is a string
        // created by the CMake build files.
        // recipe.Load("/workspace/recipe/barcode.precipe");
        // recipe.Load("/workspace/recipe/QR.precipe");
        recipe.Load("/wp/recipe/RecipeGR.precipe");
        // recipe.PreAllocateResources();

        // recipe.GetParameters().Get(StringParameterName("Camera/@CameraDevice/ImageFilename")).SetValue("/workspace/qrcode");
        //  For demonstration purposes only
        //  Let's check the Pylon::CDeviceInfo properties of the camera we are going to use.
        //  Basler recommends using the DeviceClass and the UserDefinedName to identify a camera.
        //  The UserDefinedName is taken from the DeviceUserID parameter that you can set in the pylon Viewer's Features pane.
        //  Note: USB cameras must be disconnected and reconnected or reset to provide the new DeviceUserID.
        //  This is due to restrictions defined by the USB standard.
        cout << "Properties used for selecting a camera device" << endl;
        CIntegerParameter devicePropertySelector =
            recipe.GetParameters().Get(IntegerParameterName("Camera/@vTool/DevicePropertySelector"));
        if (devicePropertySelector.IsWritable())
        {
            CStringParameter deviceKey = recipe.GetParameters().Get(StringParameterName("Camera/@vTool/DevicePropertyKey"));
            CStringParameter deviceValue = recipe.GetParameters().Get(StringParameterName("Camera/@vTool/DevicePropertyValue"));
            for (int64_t i = devicePropertySelector.GetMin(); i <= devicePropertySelector.GetMax(); ++i)
            {
                devicePropertySelector.SetValue(i);
                cout << deviceKey.GetValue() << "=" << deviceValue.GetValue() << endl;
            }
        }
        else
        {
            cout << "The first camera device found is used." << endl;
        }

        // For demonstration purposes only
        // Print available parameters.
        //        {
        //            cout << "Parameter names before allocating resources" << endl;
        //            StringList_t parameterNames = recipe.GetParameters().GetAllParameterNames();
        //            for (const auto& name : parameterNames)
        //            {
        //                cout << name << endl;
        //            }
        //        }

        // Allocate the required resources. This includes the camera device.
        recipe.PreAllocateResources();
        // recipe.GetParameters().Get(StringParameterName("Camera/@CameraDevice/ImageFilename")).SetValue("/workspace/recipe/Emulation_0815-0000.pfs");

        // For demonstration purposes only
        cout << "Selected camera device:" << endl;
        cout << "ModelName=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceModelName")).GetValueOrDefault("N/A") << std::endl;
        cout << "SerialNumber=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceSerialNumber")).GetValueOrDefault("N/A") << std::endl;
        cout << "VendorName=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceVendorName")).GetValueOrDefault("N/A") << std::endl;
        cout << "UserDefinedName=" << recipe.GetParameters().Get(StringParameterName("Camera/@vTool/SelectedDeviceUserDefinedName")).GetValueOrDefault("N/A") << std::endl;
        // StringParameterName is the type of the parameter.
        // MyCamera is the name of the vTool.
        // Use @vTool if you want to access the vTool parameters.
        // Use @CameraInstance if you want to access the parameters of the CInstantCamera object used internally.
        // Use @DeviceTransportLayer if you want to access the transport layer parameters.
        // Use @CameraDevice if you want to access the camera device parameters.
        // Use @StreamGrabber0 if you want to access the camera device parameters.
        // SelectedDeviceUserDefinedName is the name of the parameter.

        // For demonstration purposes only
        // Print available parameters after allocating resources. Now we can access the camera parameters.
        //        {
        //            cout << "Parameter names after allocating resources" << endl;
        //            StringList_t parameterNames = recipe.GetParameters().GetAllParameterNames();
        //            for (const auto& name : parameterNames)
        //            {
        //                cout << name << endl;
        //            }
        //        }

        // For demonstration purposes only
        // Print available output names.
        StringList_t outputNames;
        recipe.GetOutputNames(outputNames);
        for (const auto &outputName : outputNames)
        {
            cout << "Output found: " << outputName << std::endl;
        }

        // Register the helper object for receiving all output data.
        recipe.RegisterAllOutputsObserver(&resultCollector, RegistrationMode_Append);

        // Start the processing. The recipe is triggered internally
        // by the camera vTool for each image.
        recipe.Start();
        CEnumParameter testImageSelector =
            recipe.GetParameters().Get(EnumParameterName("Camera/@CameraDevice/TestImageSelector"));

        while (!done)
        {
            CGrabResultPtr gr;
            m_mutx.lock();
            if (m_grabResults.size() > 0)
            {
                gr = m_grabResults[0];
                m_grabResults.erase(m_grabResults.begin());
            }
            m_mutx.unlock();

            if (gr->GrabSucceeded())
            {
                CPylonImage srcImage;
                srcImage.AttachGrabResultBuffer(gr);

                CVariant srcVariant(srcImage);
                recipe.TriggerUpdateAsync("GRImageImput", srcVariant, nullptr, gr->GetBlockID());

                if (resultCollector.GetWaitObject().Wait(5000))
                {
                    ResultData result;
                    resultCollector.GetResultData(result);
                    if (!result.hasError)
                    {
                        // Access the image data.
                        cout << "SizeX: " << result.image.GetWidth() << endl;
                        cout << "SizeY: " << result.image.GetHeight() << endl;
                        cout << "image count " << gr->GetBlockID() << endl;
                        // const uint8_t* pImageBuffer = (uint8_t*)result.image.GetBuffer();
                        // cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;
                        int i = 1;
                        for (auto val : result.decodedBarcodes)
                        {

                            cout << "found text : " << val << endl;
                        }

                        // m_fc.OutputPixelFormat = PixelType_RGB8packed;
                        // m_fc.Convert(image, result.image);

                        // Mat cv_img(result.image.GetHeight(), result.image.GetWidth(), CV_8U, (uint8_t *)result.image.GetBuffer());
                        // Mat cv_img_small;
                        // cv::resize(cv_img, cv_img_small, cv::Size(), 0.5, 0.5);
                        // imshow(windowsName, cv_img_small);
                        //  if ((waitKey(5) & 0xEFFFFF) == 27)
                        //  {
                        //      done = true;
                        //  }
                    }
                    else
                    {
                        cout << "An error occurred in processing: " << result.errorMessage << endl;
                    }
                }
                else
                {
                    throw RUNTIME_EXCEPTION("Result timeout");
                }
            }
            else
            {
                cout << "no Grabresult aviable " << endl;
            }
        }
        destroyAllWindows();
        // Stop the processing.
        recipe.Stop();

        // Optionally, deallocate resources.
        recipe.DeallocateResources();
    }
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
             << e.GetDescription() << endl;
        exitCode = 1;
    }
    return exitCode;
}
bool imageprocessor::stop()
{
    done = true;
    return done;
}
std::thread imageprocessor::startAsync(const char *name, unsigned pos)
{
    return std::thread([=]
                       { doBarcodeeading(/*arg1, arg2*/); });
}