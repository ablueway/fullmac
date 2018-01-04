#ifndef _DRV_L1_SPU_MIDI_H_
#define _DRV_L1_SPU_MIDI_H_

#include "driver_l1.h"

#define MODE_LOOP        0x00
#define MODE_JUMP        0x80

#define B10000000               0x80
#define B01000000               0x40
#define B00100000               0x20
#define B00010000               0x10
#define B00001000               0x08
#define B00000100               0x04
#define B00000010               0x02
#define B00000001               0x01

#define C_SPU_ADPCM_4BIT        0x00
#define C_SPU_ADPCM_5BIT        0x40
#define C_SPU_PCM_8BIT          0x80
#define C_SPU_PCM_16BIT         0xC0

#define C_MODE_LOOP             0x00
#define C_MODE_JUMP             0x80


#define C_INST_SECINFO_WORD_SIZE         9
#define C_INST_SECINFO_PITCH_WORD        0
#define C_INST_SECINFO_ENV_OFFSET_WORD   1
#define C_INST_SECINFO_ENV_SIZE_WORD     2
#define C_INST_SECINFO_ATTACK_ADDR_WORD  3
#define C_INST_SECINFO_ATTACK_SIZE_WORD  4
#define C_INST_SECINFO_LOOP_ADDR_WORD    5
#define C_INST_SECINFO_LOOP_SIZE_WORD    6
#define C_INST_SECINFO_NOTEOFF_WORD      7
#define C_INST_SECINFO_COMB_WORD         8

#define C_DRUM_INFO_PITCH_WORD           0
#define C_DRUM_INFO_ATTACK_ADDR_WORD     1
#define C_DRUM_INFO_ATTACK_SIZE_WORD     2
#define C_DRUM_INFO_ENV_OFFSET_WORD      3
#define C_DRUM_INFO_ENV_SIZE_WORD        4
#define C_DRUM_INFO_NOTEOFF_WORD         5
#define C_DRUM_INFO_LOOP_ADDR_WORD       6
#define C_DRUM_INFO_LOOP_SIZE_WORD       7
#define C_DRUM_INFO_COMB_WORD            8

#define C_SAMPLE_RATE_NUM_BYTE_SIZE      4
#define C_SAMPLE_RATE_INFO_BYTE_SIZE     16
#define C_SAMPLE_RATE_DOWN_OFFSET_WORD   0
#define C_SAMPLE_RATE_DOWN_SIZE_WORD     1
#define C_SAMPLE_RATE_UP_OFFSET_WORD     2
#define C_SAMPLE_RATE_UP_SIZE_WORD       3

// Actually, MIDI default temp=100, max=250
#define C_TempoIndexBegin       10              // 20~10: tempo up
#define C_TempoIndexOffset      20              // Defaule tempo
#define C_TempoIndexEnd         30              // 20~30: tempo down

#define C_DefaultVolumeLevel    24              // Default MIDI software volume control level
#define C_MinVolumeLevel        31
#define C_DefaultPBLevel        30              // Default MIDI Pitch Bend level
#define C_EndPBLevel            64

#define C_S1HWindowValue        20

#define C_TimerCRate            0               // TimerC interrupt service rate control(0:1ms, 1:2ms, 2:4ms)

//==================================================================================
// MIDI Status
//==================================================================================
#define  D_MIDIEn               B10000000       // MIDI is enabled
#define  D_DeltaTime            B01000000       // Bit6 = 1, not Delta-time data
#define  D_MusicOff             B00100000       // Music off
#define  D_PlaySingleNote       B00010000       // Unused
#define  D_DynaAllocOff         B00001000       // D_DynaAllocOff = 1, dynamic allocation Off, otherwise dynamic allocation on
#define  D_NewestNoteOut        B00000100       // D_NewestNoteOut = 1, Newest note out; D_NewestNoteOut = 0, check D_MinVolNoteOut
#define  D_MinVolNoteOut        B00000010       // D_MinVolNoteOut = 1, minimum volume note out; D_MinVolNoteOut = 0, oldest note out
#define  D_UserEvent            B00000001       // Indicate a MIDI event is detected

//==================================================================================
// Channel Status definition
//==================================================================================
#define   D_NotePlaying         B10000000       // Indicate a note is played
#define   D_NoteOffFlag         B01000000       // Indicate a note is released
#define   D_AdpcmJumpPcm        B00100000       // Indicate a note is ADPCM+PCM
#define   D_DrumPlaying         B00010000       // Indicate a drum note is played
#define   D_TargetIsLastDrop    B00001000       // Indicate the current target is the last drop in the envelope table
#define   D_ReachLastDrop       B00000100       // Indicate the envelope is reach the last drop
#define   D_SkipIniNoteOffEnv   B00000010       // Indicate the envelope of note off is initialized
#define   D_ZCJump              B00000001       // Indicate a note is set as Zero Corssing
//==================================================================================
#define   D_PauseMIDI           B00000001       // Used to pause a MIDI
#define   D_MIDIEvent           B00000010       // Indicate a MIDI event is detected
#define   D_EnvUpdate           B00000001

#define   D_DisMIDICh1          B00000001
#define   D_DisMIDICh2          B00000010
#define   D_DisMIDICh3          B00000100
#define   D_DisMIDICh4          B00001000
#define   D_DisMIDICh5          B00010000
#define   D_DisMIDICh6          B00100000
#define   D_DisMIDICh7          B01000000
#define   D_DisMIDICh8          B10000000
#define   D_DisMIDICh9          B00000001
#define   D_DisMIDICh10         B00000010
#define   D_DisMIDICh11         B00000100
#define   D_DisMIDICh12         B00001000
#define   D_DisMIDICh13         B00010000
#define   D_DisMIDICh14         B00100000
#define   D_DisMIDICh15         B01000000
#define   D_DisMIDICh16         B10000000
#define   D_CtrlCh1Vol          B00000001
#define   D_CtrlCh2Vol          B00000010
#define   D_CtrlCh3Vol          B00000100
#define   D_CtrlCh4Vol          B00001000
#define   D_CtrlCh5Vol          B00010000
#define   D_CtrlCh6Vol          B00100000
#define   D_CtrlCh7Vol          B01000000
#define   D_CtrlCh8Vol          B10000000
#define   D_CtrlCh9Vol          B00000001
#define   D_CtrlCh10Vol         B00000010
#define   D_CtrlCh11Vol         B00000100
#define   D_CtrlCh12Vol         B00001000
#define   D_CtrlCh13Vol         B00010000
#define   D_CtrlCh14Vol         B00100000
#define   D_CtrlCh15Vol         B01000000
#define   D_CtrlCh16Vol         B10000000
#define   D_RepeatMIDI          B10000000       // Bit7 is a Repeat On/Off flag

extern void SPU_MIDI_SetTimeBase(INT8U R_SPU_MIDI_Adsr, INT8U R_SPU_MIDI_NoteOff);
extern void SPU_MIDI_Play(INT32U* addr_gmd_res, INT8U index_midi);
extern void SPU_MIDI_SetDynaMode(INT8U mode_sel_dyna);
extern INT8U SPU_MIDI_GetStatus(void);
extern void SPU_MIDI_Pause(void);
extern void SPU_MIDI_Resume(void);
extern void SPU_MIDI_Stop(void);
extern void SPU_MIDI_SetRepeat(INT8U repeat_en);
extern void SPU_MIDI_SetSWVol(INT8U sw_vol);
extern void SPU_MIDI_TempoUp(void);
extern void SPU_MIDI_TempoDn(void);
extern void SPU_MIDI_ChangeInst(INT8U midi_ch_index, INT8U R_SPU_MIDI_InstrumentIndex);
extern void SPU_MIDI_SetCHMute(INT8U midi_ch_index, INT8U mute_en);
extern void SPU_MIDI_KeyShiftUp(void);
extern void SPU_MIDI_KeyShiftDn(void);
extern INT16U SPU_MIDI_GetUserEvent(void);
extern INT8U SPU_MIDI_GetMIDIEvent(void);
extern void SPU_MIDI_PitchBendUp(void);
extern void SPU_MIDI_PitchBendDn(void);
extern void SPU_MIDI_SetTonePitchBend(INT8U spu_ch);
extern void SPU_SingleNote_Play(INT8U spu_ch, INT8U note_pitch, INT8U note_inst, INT8U note_vol);
extern void SPU_SingleDrum_Play(INT8U spu_ch, INT8U drum_pitch, INT8U drum_inst, INT8U drum_vol);
extern void SPU_SingleNote_OFF(INT8U spu_ch);
extern INT32U SPU_MIDI_GetGMDMidiNums(INT32U* addr_gmd_res);

extern void SPU_MIDI_DecodeService(void);
extern void SPU_MIDI_DurationService(void);
extern void SPU_MIDI_ADSRService(void);
extern void SPU_MIDI_DataCounter(void);
extern void SPU_MIDI_PlayTone(INT8U INT_Conc, INT8U spu_ch);
extern void SPU_MIDI_PlayDrum(INT8U INT_Conc, INT8U spu_ch);
extern void SPU_MIDI_SetDynamic(INT8U midi_dyna_mode);
extern void SPU_MIDI_ChADSRService(INT8U spu_ch);
extern INT8U* SPU_MIDI_GetToneAdsr(INT8U spu_ch);
extern INT8U* SPU_MIDI_GetDrumAdsr(INT8U spu_ch);
extern INT32U* SPU_MIDI_GetToneAddr(INT8U spu_ch);
extern INT32U* SPU_MIDI_GetDrumAddr(INT8U spu_ch);
extern INT8U SPU_MIDI_FindOldestNoteCh(INT8U spu_ch);
extern INT8U SPU_MIDI_FindNewestNoteCh(INT8U spu_ch);
extern INT8U SPU_MIDI_FindMinVolCh(INT8U spu_ch);
extern void SPU_MIDI_StopChVol(INT8U spu_ch);
extern void SPU_MIDI_CalculateChVol(INT8U spu_ch);
extern void SPU_MIDI_ChNoteOffService(INT8U spu_ch);
extern void SPU_MIDI_InitNoteOff(INT8U spu_ch);
extern void SPU_MIDI_SetChIrqPara(INT8U spu_ch);
extern void SPU_MIDI_SetTonePitchBend(INT8U spu_ch);
extern void SPU_MIDI_GetEnvolope(INT8U* tab_ptr, INT8U spu_ch, INT8U vol_skip);
extern void SPU_MIDI_ChVolCtrlEn(INT8U midi_ch_index);
extern void SPU_MIDI_CalculateChVol(INT8U spu_ch);

extern INT8U SPU_MIDI_FindDurationNoteCh(INT8U spu_ch);
extern void SPU_Init(void);
extern void SPU_MIDI_Register_Callback(void (*end)(void));
#endif