#include "OSXAudioLoader.h"
#include <AudioToolbox/AudioToolbox.h>
#include <iostream>

OSXAudioLoader::OSXAudioLoader()
{
    device = alcOpenDevice(NULL);
    if (device != nullptr)
    {
        context = alcCreateContext(device, 0);
        if (context != nullptr)
        {
            alcMakeContextCurrent(context);
            
            initialized = true;
        }
    }
    
    //alGetError();
}

OSXAudioLoader::~OSXAudioLoader()
{
    alcDestroyContext(context);
    alcCloseDevice(device);
    
    context = nullptr;
    device = nullptr;
    
    initialized = false;
}

bool OSXAudioLoader::loadFromMP3(const std::string &path, AudioInfo& info)
{
    return false;
}

bool OSXAudioLoader::loadFromOGG(const std::string &path, AudioInfo& info)
{
    return false;
}

bool OSXAudioLoader::loadFromWAV(const std::string &path, AudioInfo& info)
{
    OSStatus error = noErr;
    AudioFileID fileID = 0;
    AudioStreamBasicDescription properties;
    UInt32 propertiesSize = sizeof(AudioStreamBasicDescription);
    UInt64 dataSize = 0;
    
    CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, cfPath, NULL);
    
    error = AudioFileOpenURL(url, kAudioFileReadPermission, 0, &fileID);
    if (error)
    {
        std::cout << "Cannot open the specified file" << std::endl;
        
        CFRelease(url);
        CFRelease(cfPath);
        
        return false;
    }
    
    error = AudioFileGetProperty(fileID, kAudioFilePropertyDataFormat, &propertiesSize, &properties);
    if (error)
    {
        std::cout << "Cannot get the properties of the specified file" << std::endl;
        
        AudioFileClose(fileID);
        
        CFRelease(url);
        CFRelease(cfPath);
        
        return false;
    }
    
    info.channelsCount = properties.mChannelsPerFrame;
    info.sampleRate = properties.mSampleRate;
    
    propertiesSize = sizeof(dataSize);
    error = AudioFileGetProperty(fileID, kAudioFilePropertyAudioDataByteCount, &propertiesSize, &dataSize);
    if (error)
    {
        std::cout << "Cannot get the size of the specified file" << std::endl;
        
        AudioFileClose(fileID);
        
        CFRelease(url);
        CFRelease(cfPath);
        
        return false;
    }
    
    if (info.samples != nullptr)
        info.samples->clear();
    else
        info.samples = new std::vector<int16_t>();
    
    info.samples->resize(dataSize);
    
    UInt32 size = (UInt32)dataSize;
    error = AudioFileReadBytes(fileID, false, 0, &size, &info.samples->at(0));
    if (error)
    {
        std::cout << "Cannot read the raw byte of the specified file" << std::endl;
        
        AudioFileClose(fileID);
        
        CFRelease(url);
        CFRelease(cfPath);
        
        return false;
    }
    
    if (fileID)
        AudioFileClose(fileID);
    fileID = 0;
    
    CFRelease(url);
    CFRelease(cfPath);
    
    return true;
}







