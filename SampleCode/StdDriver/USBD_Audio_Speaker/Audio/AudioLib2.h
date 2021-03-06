#ifndef __AUDIOLIB2_H__
#define __AUDIOLIB2_H__

#ifdef __cplusplus
extern "C" {
#endif

void _UAC_MicCopyTo16(S_AUDIO_LIB* psAudioLib);
void _UAC_MicCopyTo32(S_AUDIO_LIB* psAudioLib);
void _UAC_MicCopyTo16_2(S_AUDIO_LIB* psAudioLib);

void _UAC_MicSendTo16to16(S_AUDIO_LIB* psAudioLib);
void _UAC_MicSendTo24to24(S_AUDIO_LIB* psAudioLib);
void _UAC_MicSendTo32to32(S_AUDIO_LIB* psAudioLib);
void _UAC_MicSendTo16to16_2(S_AUDIO_LIB* psAudioLib);
void _UAC_MicSendTo16to24(S_AUDIO_LIB* psAudioLib);
void _UAC_MicSendTo16to32(S_AUDIO_LIB* psAudioLib);

void _UAC_MicStop_2(S_AUDIO_LIB* psAudioLib);

void _UAC_SpkCopyFrom16(S_AUDIO_LIB* psAudioLib);
void _UAC_SpkCopyFrom32(S_AUDIO_LIB* psAudioLib);
void _UAC_SpkCopyFrom16_2(S_AUDIO_LIB* psAudioLib);

void _UAC_SpkRecvFrom16to16(S_AUDIO_LIB* psAudioLib, int32_t i32Len);
void _UAC_SpkRecvFrom24to24(S_AUDIO_LIB* psAudioLib, int32_t i32Len);
void _UAC_SpkRecvFrom32to32(S_AUDIO_LIB* psAudioLib, int32_t i32Len);
void _UAC_SpkRecvFrom16to16_2(S_AUDIO_LIB* psAudioLib, int32_t i32Len);
void _UAC_SpkRecvFrom24to16(S_AUDIO_LIB* psAudioLib, int32_t i32Len);
void _UAC_SpkRecvFrom32to16(S_AUDIO_LIB* psAudioLib, int32_t i32Len);

#ifdef __cplusplus
}
#endif

#endif  /* __AUDIOLIB2_H__ */
