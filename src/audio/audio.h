#pragma once
#include "platform.h"
#include "sol/base.h"

// ma_context context;
// if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS)
// {
//     // Error.
// }

// ma_device_info *pPlaybackInfos;
// ma_uint32       playbackCount;
// ma_device_info *pCaptureInfos;
// ma_uint32       captureCount;
// if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
// {
//     // Error.
// }
//     for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1) {
//     printf("%d - %s\n", iDevice, pPlaybackInfos[iDevice].name);
// }
// ma_device_config config = ma_device_config_init(ma_device_type_playback);
// config.playback.pDeviceID = &pPlaybackInfos[3].id;
// config.playback.channels  = SOL_CHANNELS;
// config.sampleRate         = SOL_SAMPLERATE;

// ma_device device;
// if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
//     // Error
// }

// ma_device_uninit(&device);
// ma_context_uninit(&context);

// Sol_Load_Audios();