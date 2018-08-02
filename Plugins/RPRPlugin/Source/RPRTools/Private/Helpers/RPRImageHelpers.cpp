#include "Helpers/RPRImageHelpers.h"
#include "Helpers/RPRHelpers.h"
#include "Helpers/GenericGetInfo.h"
#include "RenderUtils.h"

DECLARE_LOG_CATEGORY_CLASS(LogRPRImageHelpers, Log, All)

namespace RPR
{
	namespace Image
	{
		template<typename T>
		RPR::FResult GetInfoNoAlloc(RPR::FImage Image, RPR::EImageInfo ImageInfo, T* OutValue)
		{
			return RPR::Generic::GetInfoNoAlloc(rprImageGetInfo, Image, ImageInfo, OutValue);
		}

		template<typename T>
		RPR::FResult GetInfoToArray(RPR::FImage Image, RPR::EImageInfo ImageInfo, TArray<T>& OutValue)
		{
			return RPR::Generic::GetInfoToArray(rprImageGetInfo, Image, ImageInfo, OutValue);
		}

		//////////////////////////////////////////////////////////////////////////

		RPR::FResult GetFormat(RPR::FImage Image, EPixelFormat& OutFormat)
		{
			FImageFormat imageFormat;
			RPR::FResult status = GetInfoNoAlloc(Image, EImageInfo::Format, &imageFormat);
			if (RPR::IsResultSuccess(status))
			{
				if (!ConvertRPRImageFormatToUE4PixelFormat(imageFormat, OutFormat))
				{
					return RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT;
				}
			}
			return (status);
		}

		bool ConvertRPRImageFormatToUE4PixelFormat(RPR::FImageFormat ImageFormat, EPixelFormat& OutPixelFormat)
		{
			RPR::EComponentType componentType = (RPR::EComponentType) ImageFormat.type;

			if (ImageFormat.num_components == 3 && componentType == EComponentType::Float32)
			{
				OutPixelFormat = PF_FloatR11G11B10;
				return (true);
			}
			if (ImageFormat.num_components == 4 && componentType == RPR::EComponentType::Uint8)
			{
				OutPixelFormat = PF_R8G8B8A8;
				return (true);
			}

			UE_LOG(LogRPRImageHelpers, Error, TEXT("Unsupported image format conversion to UE4 pixel format (num component : %d, type : %d)"), ImageFormat.num_components, ImageFormat.type);
			OutPixelFormat = PF_Unknown;
			return (false);
		}

		//////////////////////////////////////////////////////////////////////////

		RPR::FResult GetDescription(RPR::FImage Image, FImageDesc& OutDescription)
		{
			return GetInfoNoAlloc(Image, EImageInfo::Description, &OutDescription);
		}

		RPR::FResult GetBufferData(RPR::FImage Image, TArray<uint8>& OutBuffer)
		{
			return GetInfoToArray(Image, EImageInfo::Data, OutBuffer);
		}

		RPR::FResult GetWrapMode(RPR::FImage Image, RPR::EImageWrapType& OutWrapMode)
		{
			return GetInfoNoAlloc(Image, EImageInfo::WrapMode, &OutWrapMode);
		}

		RPR::FResult GetFilterMode(RPR::FImage Image, RPR::EImageFilterType& OutFilterMode)
		{
			return GetInfoNoAlloc(Image, EImageInfo::FilterMode, &OutFilterMode);
		}

		RPR::FResult GetGammaValue(RPR::FImage Image, float& GammaValue)
		{
			return GetInfoNoAlloc(Image, EImageInfo::Gamma, &GammaValue);
		}

		RPR::FResult IsMipMapEnabled(RPR::FImage Image, bool& bIsMipMapEnabled)
		{
			return GetInfoNoAlloc(Image, EImageInfo::MipMapEnabled, &bIsMipMapEnabled);
		}

	} // namespace Image
} // namespace RPR