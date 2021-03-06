
#include "CamaroMovidius.h"
#include <thread>
#include <array>
#include <cmath>
#include "VideoFrameEx.h"

using namespace TopGear;

int CamaroMovidius::Flip(bool vertical, bool horizontal)
{
    auto result = SetControl("Flip", PropertyData<uint8_t>((vertical?0x01:0x00)|(horizontal?0x02:0x00)));
    return result ? 0:-1;
}

bool CamaroMovidius::SetControl(std::string name, IPropertyData &&val)
{
    auto& obj = val;
    return SetControl(name, obj);
}

bool CamaroMovidius::SetControl(std::string name, IPropertyData &val)
{
    if (&config == &CameraProfile::NullObject())
        return false;
    auto it = config.XuControls.find(name);
    if (it != config.XuControls.end())
    {
        if (it->second.TypeHash != val.GetTypeHash())
            return false;
        if (it->second.Attribute.find('w') == std::string::npos)
            return false;
        //Find fixed prefix
        auto pit = it->second.Payloads.find("w");
        size_t prefixLen = 0;
        uint8_t *pPrefix = nullptr;
        if (pit != it->second.Payloads.end())
        {
            pPrefix = pit->second.second.data();
            prefixLen = pit->second.second.size();
        }

        if (val.GetTypeHash() == typeid(uint8_t).hash_code())
            return extensionAdapter.SetProperty<uint8_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(uint16_t).hash_code())
            return extensionAdapter.SetProperty<uint16_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(int).hash_code())
            return extensionAdapter.SetProperty<int32_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(std::string).hash_code())
            return extensionAdapter.SetProperty<std::string>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(std::vector<uint8_t>).hash_code())
            return extensionAdapter.SetProperty<std::vector<uint8_t>>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
    }
    return false;
}
bool CamaroMovidius::GetControl(std::string name, IPropertyData &val)
{
    if (&config == &CameraProfile::NullObject())
        return false;
    auto it = config.XuControls.find(name);
    if (it != config.XuControls.end())
    {
        if (it->second.TypeHash != val.GetTypeHash())
            return false;
        if (it->second.Attribute.find('r') == std::string::npos)
            return false;
        //Find fixed prefix
        auto pit = it->second.Payloads.find("r");
        size_t prefixLen = 0;
        uint8_t *pPrefix = nullptr;
        if (pit != it->second.Payloads.end())
        {
            if (pit->second.first)	//Need verify prefix
                pPrefix = pit->second.second.data();
            prefixLen = pit->second.second.size();
        }

        if (val.GetTypeHash() == typeid(uint8_t).hash_code())
            return extensionAdapter.GetProperty<uint8_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(uint16_t).hash_code())
            return extensionAdapter.GetProperty<uint16_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(int).hash_code())
            return extensionAdapter.GetProperty<int32_t>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(std::string).hash_code())
            return extensionAdapter.GetProperty<std::string>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
        if (val.GetTypeHash() == typeid(std::vector<uint8_t>).hash_code())
            return extensionAdapter.GetProperty<std::vector<uint8_t>>(it->second.Code, val, pPrefix, prefixLen, it->second.IsBigEndian);
    }
    return false;
}

int CamaroMovidius::GetExposure(bool &ae, float &ev)
{
    ev = 1.0f;
    auto result = -1;
    try
    {
        PropertyData<uint8_t> data;
        if (GetControl("AutoExposure", data))
        {
            ae = data.Payload;
            if (data.Payload>1)
                ev = data.Payload/128.0;
            result = 0;
        }
    }
    catch (const std::out_of_range&)
    {
    }
    return result;
}

int CamaroMovidius::SetExposure(bool ae, float ev)
{
    auto result = -1;
    float val = ev*128;
    uint8_t ev_int = uint8_t((val>255)?255:(val<2?2:val));
    try
    {
        if (SetControl("AutoExposure", PropertyData<uint8_t>(ae?ev_int:0)))
            result = 0;
    }
    catch (const std::out_of_range&)
    {
    }
    return result;
}

int CamaroMovidius::GetShutter(uint32_t& val)
{
    PropertyData<int32_t> result;
    if (!GetControl("ShutterLimit", result))
        return -1;
    val = uint32_t(result.Payload);
    return 0;
}

int CamaroMovidius::SetShutter(uint32_t val)
{
    auto result = -1;
    bool ae;
    float ev;
    auto hr = GetExposure(ae, ev);
    if (hr>=0 && ae)    //AE enable
    {
        if (SetControl("ShutterLimit", PropertyData<int32_t>(int32_t(val))))
            result = 0;
    }
    else    //Manual Exposure
    {
    }
    return result;
}

int CamaroMovidius::GetIris(float &ratio)
{
    ratio = 0;
    auto it = config.XuControls.find("Iris");
    if (it == config.XuControls.end())
        return -1;
    auto dval = it->second.DefaultVal>>8;
    if (dval==0)
        return -1;
    PropertyData<uint16_t> result;
    if (!GetControl("Iris", result))
        return -1;
    ratio = (result.Payload>>8)*1.0f/dval;
    return 0;
}

int CamaroMovidius::SetIris(float ratio)
{
    if (ratio<0 || ratio>1.0f)
        return -1;
    auto it = config.XuControls.find("Iris");
    if (it == config.XuControls.end())
        return -1;
    auto val = uint16_t(it->second.DefaultVal);
    val = uint16_t((int((val>>8)*ratio)<<8)|((val&0xff)+irisOffset));
    return SetControl("Iris", PropertyData<uint16_t>(val))?0:-1;
}

int CamaroMovidius::SetIrisOffset(int offset)
{
    auto it = config.XuControls.find("Iris");
    if (it == config.XuControls.end())
        return -1;
    auto val = uint16_t(it->second.DefaultVal);

	int dst_val = (int) (val & 0xff) + offset;
	if (dst_val < 0 || dst_val > 0xff)
		return -1;

	irisOffset = offset;
	return 0;
}

int CamaroMovidius::GetGain(float& gainR, float& gainG, float& gainB)
{
    (void)gainR;
    (void)gainG;
    (void)gainB;
    return -1;
}

int CamaroMovidius::SetGain(float gainR, float gainG, float gainB)
{
    (void)gainR;
    (void)gainG;
    (void)gainB;
    return -1;
}

CamaroMovidius::CamaroMovidius(std::shared_ptr<IVideoStream>& vs,
                               std::shared_ptr<IExtensionAccess>& ex,
                               CameraProfile &con)
    : CameraSoloBase(vs, con), extensionAdapter(ex),
      moving(false), movingSet(false)
{
    PropertyData<std::string> info;
    if (CamaroMovidius::GetControl("DeviceInfo", info))
    {
        auto query = config.QueryRegisterMap(info.Payload);
        sensorInfo = query.first;
    }
    auto builtinFormats = pReader->GetAllFormats();
    for(auto &item : builtinFormats)
    {
        for(auto rate=10;rate<=30;rate+=5)
        {
            VideoFormat format(item);
            format.MaxRate = rate;
            formats.emplace_back(format);
        }
    }
    Flip(false, false);
}

CamaroMovidius::~CamaroMovidius()
{
    StopStream();
}

int CamaroMovidius::GetOptimizedFormatIndex(VideoFormat& format, const char* fourcc)
{
    auto bandwidth = 0;
    auto index = -1;
    auto i = -1;
    for (auto f : formats)
    {
        ++i;
        if (std::strcmp(fourcc, "") != 0 && std::strncmp(fourcc, f.PixelFormat, 4) != 0)
            continue;
        if (f.Width*f.Height*f.MaxRate > bandwidth)
            index = i;
    }
    if (index >= 0)
        format = formats[index];
    return index;
}

int CamaroMovidius::GetMatchedFormatIndex(const VideoFormat& format) const
{
    auto index = -1;
    for (auto i : formats)
    {
        index++;
        if (format.Width > 0 && format.Width != i.Width)
            continue;
        if (format.Height> 0 && format.Height != i.Height)
            continue;
        if (format.MaxRate > 0 && format.MaxRate != i.MaxRate)
            continue;
        if (std::strcmp(format.PixelFormat, "") != 0 && std::strncmp(format.PixelFormat, i.PixelFormat, 4) != 0)
            continue;
        return index;
    }
    return -1;
}

const std::vector<VideoFormat>& CamaroMovidius::GetAllFormats() const
{
    return formats;
}

const VideoFormat &CamaroMovidius::GetCurrentFormat() const
{
    if (currentFormatIndex < 0)
        return VideoFormat::Null;
    return formats[currentFormatIndex];
}

bool CamaroMovidius::SetCurrentFormat(uint32_t formatIndex)
{
    auto &format = formats[formatIndex];
    VideoFormat builtin(format);
    builtin.MaxRate = 0;
    auto index = pReader->GetMatchedFormatIndex(builtin);
    if (index<0)
        return false;
    if (!pReader->SetCurrentFormat(index))
        return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); //Need to wait, Movidius bug
    if (!SetControl("MaxRate", PropertyData<uint8_t>(format.MaxRate)))
        return false;
    currentFormatIndex = formatIndex;
    return true;
}

bool CamaroMovidius::StartStream()
{
	firstFrame = true;
	CameraSoloBase::StartStream();
}

void CamaroMovidius::PostProcess(std::vector<IVideoFramePtr> &frames)
{
	movingSet = false;
	if (firstFrame)
	{
		frames.clear();
		firstFrame = false;
		return;
	}
    if (frames.size() != 1)
	    return;
	auto frame = frames[0];
    uint8_t *pData;
	uint32_t stride;
	if (frame->LockBuffer(&pData, &stride) != 0)
		return;
//	uint8_t checksum=0;
//	for(int i=0;i<8;++i)
//		checksum += pData[i];
//	if (checksum == 0)
//	{
//		uint64_t tm = (*(uint64_t*)pData & 0x00FFFFFFFFFFFFFFULL);
//		if (tmOffset==0)
//			tmOffset = frame->GetTimestamp() - tm;
//		IVideoFramePtr ex = std::make_shared<VideoFrameEx>(frame, 0,
//			stride, frame->GetFormat().Width, frame->GetFormat().Height,
//			0, 0, 0, 0, tm+tmOffset);
//		frames.clear();
//		frames.emplace_back(ex);
//	}
	frame->UnlockBuffer();
}

void CamaroMovidius::StartMove()
{
    if (SetControl("Resync", PropertyData<uint16_t>(0)))
    {
        moving = true;
        movingSet = true;
    }
    CameraSoloBase::StartMove();
}

void CamaroMovidius::StopMove()
{
    if (SetControl("Resync", PropertyData<uint16_t>(2)))
    {
        moving = false;
        movingSet = true;
    }
    CameraSoloBase::StopMove();
}

bool CamaroMovidius::IsSteady()
{
    //Determine steady state if frame arrival after StopMove() invoke
    return (!movingSet && !moving);
}

