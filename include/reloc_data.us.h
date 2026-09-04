#ifndef _RELOC_DATA_H_
#define _RELOC_DATA_H_

/*
 * reloc_data.h — AUTO-GENERATED. Run tools/generate_reloc_stubs.py to
 * regenerate after updating tools/reloc_data_symbols.us.txt or after
 * adding new decomp sources that reference `ll*` linker symbols.
 *
 * Every symbol in tools/reloc_data_symbols.us.txt (vendored from
 * ssb-decomp-re) is mirrored here as a #define holding the real
 * numeric value, so downstream tools (generate_yamls.py,
 * generate_reloc_table.py) always see the full name table even if
 * the individual symbol isn't referenced from src/ directly.
 *
 * Any `ll*` name referenced by src/ but missing from the table is
 * emitted as `((intptr_t)0)` with a trailing STUBBED marker so it
 * can be grep'd and triaged later — search the file for 'STUBBED'.
 *
 * Each symbol is emitted as a `#define` (rather than `extern intptr_t`)
 * so it works as a compile-time integer constant inside file-scope
 * struct initialisers, which C11 does not allow for external variables.
 */

#include <stdint.h>

/* 4230 symbols from the vendored table, 1770 extra stubs from src/. */

#define llRelocFileCount ((intptr_t)2132)
#define llMNCommonFileID ((intptr_t)0x0)
#define llMNMainFileID ((intptr_t)0x1)
#define llMN1PFileID ((intptr_t)0x2)
#define llMNIndexFileID ((intptr_t)0x3)
#define llMNOptionFileID ((intptr_t)0x4)
#define llMNDataFileID ((intptr_t)0x5)
#define llMNVSModeFileID ((intptr_t)0x6)
#define llMNVSOptionsFileID ((intptr_t)0x7)
#define llMNVSItemSwitchFileID ((intptr_t)0x8)
#define llMNMessageFileID ((intptr_t)0x9)
#define llSC1PChallengerFileID ((intptr_t)0xa)
#define llSC1PIntroFileID ((intptr_t)0xb)
#define llCharacterNamesFileID ((intptr_t)0xc)
#define llBonusPictureFileID ((intptr_t)0xd)
#define llBonusPicturePlatformFileID ((intptr_t)0xe)
#define llMNScreenAdjustFileID ((intptr_t)0xf)
#define llMNCharactersFileID ((intptr_t)0x10)
#define llMNPlayersCommonFileID ((intptr_t)0x11)
#define llMNPlayersGameModesFileID ((intptr_t)0x12)
#define llMNPlayersPortraitsFileID ((intptr_t)0x13)
#define llFTEmblemSpritesFileID ((intptr_t)0x14)
#define llMNSelectCommonFileID ((intptr_t)0x15)
#define llMNPlayersSpotlightFileID ((intptr_t)0x16)
#define llMNPlayers1PModeFileID ((intptr_t)0x17)
#define llMNPlayersDifficultyFileID ((intptr_t)0x18)
#define llFTStocksZakoFileID ((intptr_t)0x19)
#define llGRWallpaperTrainingBlackFileID ((intptr_t)0x1a)
#define llGRWallpaperTrainingYellowFileID ((intptr_t)0x1b)
#define llGRWallpaperTrainingBlueFileID ((intptr_t)0x1c)
#define ll_29_FileID ((intptr_t)0x1d)
#define llMNMapsFileID ((intptr_t)0x1e)
#define llMNVSRecordMainFileID ((intptr_t)0x1f)
#define llMNDataCommonFileID ((intptr_t)0x20)
#define llMNCommonFontsFileID ((intptr_t)0x21)
#define llMNVSResultsFileID ((intptr_t)0x22)
#define llFTEmblemModelsFileID ((intptr_t)0x23)
#define llIFCommonDigitsFileID ((intptr_t)0x24)
#define llIFCommonAnnounceCommonFileID ((intptr_t)0x25)
#define llIFCommonPlayerTagsFileID ((intptr_t)0x26)
#define ll_39_FileID ((intptr_t)0x27)
#define llLBTransitionAeroplaneFileID ((intptr_t)0x28)
#define llLBTransitionCheckFileID ((intptr_t)0x29)
#define llLBTransitionGakubuthiFileID ((intptr_t)0x2a)
#define llLBTransitionKannonFileID ((intptr_t)0x2b)
#define llLBTransitionStarFileID ((intptr_t)0x2c)
#define llLBTransitionSudare1FileID ((intptr_t)0x2d)
#define llLBTransitionSudare2FileID ((intptr_t)0x2e)
#define ll_47_FileID ((intptr_t)0x2f)
#define llLBTransitionBlockFileID ((intptr_t)0x30)
#define llLBTransitionRotScaleFileID ((intptr_t)0x31)
#define llLBTransitionCurtainFileID ((intptr_t)0x32)
#define llLBTransitionCameraFileID ((intptr_t)0x33)
#define llMVCommonFileID ((intptr_t)0x34)
#define llMVOpeningPortraitsSet1FileID ((intptr_t)0x35)
#define llMVOpeningPortraitsSet2FileID ((intptr_t)0x36)
#define llMVOpeningRunFileID ((intptr_t)0x37)
#define llMVOpeningRoomScene1FileID ((intptr_t)0x38)
#define llMVOpeningRoomScene2FileID ((intptr_t)0x39)
#define llMVOpeningRoomScene3FileID ((intptr_t)0x3a)
#define llMVOpeningRoomScene4FileID ((intptr_t)0x3b)
#define llMVOpeningRunMainFileID ((intptr_t)0x3c)
#define llMVOpeningNewcomers1FileID ((intptr_t)0x3d)
#define llMVOpeningNewcomers2FileID ((intptr_t)0x3e)
#define llMVOpeningRoomTransitionFileID ((intptr_t)0x3f)
#define llMVOpeningJungleFileID ((intptr_t)0x40)
#define llMVOpeningCommonFileID ((intptr_t)0x41)
#define llMVOpeningClashWallpaperFileID ((intptr_t)0x42)
#define llMVOpeningYosterFileID ((intptr_t)0x43)
#define llMVOpeningCliffFileID ((intptr_t)0x44)
#define llMVOpeningStandoffFileID ((intptr_t)0x45)
#define llMVOpeningStandoffWallpaperFileID ((intptr_t)0x46)
#define llMVOpeningYamabukiFileID ((intptr_t)0x47)
#define llMVOpeningClashFightersFileID ((intptr_t)0x48)
#define llMVOpeningSectorFileID ((intptr_t)0x49)
#define llMVOpeningSectorWallpaperFileID ((intptr_t)0x4a)
#define llMVOpeningRunCrashFileID ((intptr_t)0x4b)
#define llMVEndingFileID ((intptr_t)0x4c)
#define llMNBackupClearFileID ((intptr_t)0x4d)
#define llMNBackupClearHeaderOptionFileID ((intptr_t)0x4e)
#define llMN1PContinueFileID ((intptr_t)0x4f)
#define llSC1PStageClear1FileID ((intptr_t)0x50)
#define llSC1PStageClear2FileID ((intptr_t)0x51)
#define llIFCommonGameStatusFileID ((intptr_t)0x52)
#define llEFCommonEffects1FileID ((intptr_t)0x53)
#define llEFCommonEffects2FileID ((intptr_t)0x54)
#define llEFCommonEffects3FileID ((intptr_t)0x55)
#define ll_86_FileID ((intptr_t)0x56)
#define llIFCommonItemFileID ((intptr_t)0x57)
#define llStageDreamLandFileID ((intptr_t)0x58)
#define llStageZebesFileID ((intptr_t)0x59)
#define llMVOpeningRoomWallpaperFileID ((intptr_t)0x5a)
#define ll_91_FileID ((intptr_t)0x5b)
#define llStageJungleFileID ((intptr_t)0x5c)
#define llStageYoshiFileID ((intptr_t)0x5d)
#define llStagePokemonFileID ((intptr_t)0x5e)
#define llStageCastleFileID ((intptr_t)0x5f)
#define ll_96_FileID ((intptr_t)0x60)
#define ll_97_FileID ((intptr_t)0x61)
#define ll_98_FileID ((intptr_t)0x62)
#define llStageSectorFileID ((intptr_t)0x63)
#define ll_100_FileID ((intptr_t)0x64)
#define ll_101_FileID ((intptr_t)0x65)
#define ll_102_FileID ((intptr_t)0x66)
#define ll_103_FileID ((intptr_t)0x67)
#define ll_104_FileID ((intptr_t)0x68)
#define ll_105_FileID ((intptr_t)0x69)
#define ll_106_FileID ((intptr_t)0x6a)
#define ll_107_FileID ((intptr_t)0x6b)
#define ll_108_FileID ((intptr_t)0x6c)
#define ll_109_FileID ((intptr_t)0x6d)
#define ll_110_FileID ((intptr_t)0x6e)
#define ll_111_FileID ((intptr_t)0x6f)
#define ll_112_FileID ((intptr_t)0x70)
#define ll_113_FileID ((intptr_t)0x71)
#define ll_114_FileID ((intptr_t)0x72)
#define ll_115_FileID ((intptr_t)0x73)
#define ll_116_FileID ((intptr_t)0x74)
#define ll_117_FileID ((intptr_t)0x75)
#define ll_118_FileID ((intptr_t)0x76)
#define ll_119_FileID ((intptr_t)0x77)
#define ll_120_FileID ((intptr_t)0x78)
#define ll_121_FileID ((intptr_t)0x79)
#define ll_122_FileID ((intptr_t)0x7a)
#define ll_123_FileID ((intptr_t)0x7b)
#define ll_124_FileID ((intptr_t)0x7c)
#define ll_125_FileID ((intptr_t)0x7d)
#define ll_126_FileID ((intptr_t)0x7e)
#define ll_127_FileID ((intptr_t)0x7f)
#define ll_128_FileID ((intptr_t)0x80)
#define ll_129_FileID ((intptr_t)0x81)
#define ll_130_FileID ((intptr_t)0x82)
#define ll_131_FileID ((intptr_t)0x83)
#define ll_132_FileID ((intptr_t)0x84)
#define ll_133_FileID ((intptr_t)0x85)
#define ll_134_FileID ((intptr_t)0x86)
#define ll_135_FileID ((intptr_t)0x87)
#define llBonus2CommonFileID ((intptr_t)0x88)
#define ll_137_FileID ((intptr_t)0x89)
#define ll_138_FileID ((intptr_t)0x8a)
#define ll_139_FileID ((intptr_t)0x8b)
#define ll_140_FileID ((intptr_t)0x8c)
#define ll_141_FileID ((intptr_t)0x8d)
#define ll_142_FileID ((intptr_t)0x8e)
#define ll_143_FileID ((intptr_t)0x8f)
#define ll_144_FileID ((intptr_t)0x90)
#define ll_145_FileID ((intptr_t)0x91)
#define ll_146_FileID ((intptr_t)0x92)
#define ll_147_FileID ((intptr_t)0x93)
#define ll_148_FileID ((intptr_t)0x94)
#define ll_149_FileID ((intptr_t)0x95)
#define ll_150_FileID ((intptr_t)0x96)
#define llSC1PStageClear3FileID ((intptr_t)0x97)
#define ll_152_FileID ((intptr_t)0x98)
#define ll_153_FileID ((intptr_t)0x99)
#define ll_154_FileID ((intptr_t)0x9a)
#define ll_155_FileID ((intptr_t)0x9b)
#define ll_156_FileID ((intptr_t)0x9c)
#define ll_157_FileID ((intptr_t)0x9d)
#define ll_158_FileID ((intptr_t)0x9e)
#define ll_159_FileID ((intptr_t)0x9f)
#define ll_160_FileID ((intptr_t)0xa0)
#define llFoxSpecial3FileID ((intptr_t)0xa1)
#define ll_162_FileID ((intptr_t)0xa2)
#define llFTManagerCommonFileID ((intptr_t)0xa3)
#define llIFCommonPlayerDamageFileID ((intptr_t)0xa4)
#define llIFCommonTimerFileID ((intptr_t)0xa5)
#define llIFCommonPlayerFileID ((intptr_t)0xa6)
#define llMNTitleFileID ((intptr_t)0xa7)
#define llMNTitleFireAnimFileID ((intptr_t)0xa8)
#define llMNNoControllerFileID ((intptr_t)0xa9)
#define llMNCongraKirbyBottomFileID ((intptr_t)0xaa)
#define llMNCongraKirbyTopFileID ((intptr_t)0xab)
#define llMNCongraYoshiBottomFileID ((intptr_t)0xac)
#define llMNCongraYoshiTopFileID ((intptr_t)0xad)
#define llMNCongraPikachuBottomFileID ((intptr_t)0xae)
#define llMNCongraPikachuTopFileID ((intptr_t)0xaf)
#define llMNCongraSamusBottomFileID ((intptr_t)0xb0)
#define llMNCongraSamusTopFileID ((intptr_t)0xb1)
#define llMNCongraLinkBottomFileID ((intptr_t)0xb2)
#define llMNCongraLinkTopFileID ((intptr_t)0xb3)
#define llMNCongraPurinBottomFileID ((intptr_t)0xb4)
#define llMNCongraPurinTopFileID ((intptr_t)0xb5)
#define llMNCongraCaptainBottomFileID ((intptr_t)0xb6)
#define llMNCongraCaptainTopFileID ((intptr_t)0xb7)
#define llMNCongraDonkeyBottomFileID ((intptr_t)0xb8)
#define llMNCongraDonkeyTopFileID ((intptr_t)0xb9)
#define llMNCongraMarioBottomFileID ((intptr_t)0xba)
#define llMNCongraMarioTopFileID ((intptr_t)0xbb)
#define llMNCongraLuigiBottomFileID ((intptr_t)0xbc)
#define llMNCongraLuigiTopFileID ((intptr_t)0xbd)
#define llMNCongraFoxBottomFileID ((intptr_t)0xbe)
#define llMNCongraFoxTopFileID ((intptr_t)0xbf)
#define llMNCongraNessBottomFileID ((intptr_t)0xc0)
#define llMNCongraNessTopFileID ((intptr_t)0xc1)
#define llN64LogoFileID ((intptr_t)0xc2)
#define llSCStaffrollFileID ((intptr_t)0xc3)
#define llMNSoundTestFileID ((intptr_t)0xc4)
#define llIFCommonBattlePauseFileID ((intptr_t)0xc5)
#define llSCExplainGraphicsFileID ((intptr_t)0xc6)
#define llSYKseg1ValidateFileID ((intptr_t)0xc7)
#define llSYSignValidateFileID ((intptr_t)0xc8)
#define ll_201_FileID ((intptr_t)0xc9)
#define llMarioMainMotionFileID ((intptr_t)0xca)
#define llMarioMainFileID ((intptr_t)0xcb)
#define llMarioSpecial1FileID ((intptr_t)0xcc)
#define llMMarioMainMotionFileID ((intptr_t)0xcd)
#define llMMarioMainFileID ((intptr_t)0xce)
#define llNMarioMainFileID ((intptr_t)0xcf)
#define llFoxMainMotionFileID ((intptr_t)0xd0)
#define llFoxMainFileID ((intptr_t)0xd1)
#define llFoxSpecial1FileID ((intptr_t)0xd2)
#define llNFoxMainFileID ((intptr_t)0xd3)
#define llDonkeyMainMotionFileID ((intptr_t)0xd4)
#define llDonkeyMainFileID ((intptr_t)0xd5)
#define llNDonkeyMainFileID ((intptr_t)0xd6)
#define llGDonkeyMainFileID ((intptr_t)0xd7)
#define llSamusMainMotionFileID ((intptr_t)0xd8)
#define llSamusMainFileID ((intptr_t)0xd9)
#define llSamusSpecial1FileID ((intptr_t)0xda)
#define llNSamusMainFileID ((intptr_t)0xdb)
#define llLuigiMainMotionFileID ((intptr_t)0xdc)
#define llLuigiMainFileID ((intptr_t)0xdd)
#define llLuigiSpecial1FileID ((intptr_t)0xde)
#define llNLuigiMainFileID ((intptr_t)0xdf)
#define llLinkMainMotionFileID ((intptr_t)0xe0)
#define llLinkMainFileID ((intptr_t)0xe1)
#define llLinkSpecial1FileID ((intptr_t)0xe2)
#define llNLinkMainFileID ((intptr_t)0xe3)
#define llKirbyMainMotionFileID ((intptr_t)0xe4)
#define llKirbyMainFileID ((intptr_t)0xe5)
#define ll_230_FileID ((intptr_t)0xe6)
#define llNKirbyMainFileID ((intptr_t)0xe7)
#define llPurinMainMotionFileID ((intptr_t)0xe8)
#define llPurinMainFileID ((intptr_t)0xe9)
#define llNPurinMainFileID ((intptr_t)0xea)
#define llCaptainMainMotionFileID ((intptr_t)0xeb)
#define llCaptainMainFileID ((intptr_t)0xec)
#define llNCaptainMainFileID ((intptr_t)0xed)
#define llNessMainMotionFileID ((intptr_t)0xee)
#define llNessMainFileID ((intptr_t)0xef)
#define llNessSpecial1FileID ((intptr_t)0xf0)
#define llNNessMainFileID ((intptr_t)0xf1)
#define llPikachuMainMotionFileID ((intptr_t)0xf2)
#define llPikachuMainFileID ((intptr_t)0xf3)
#define llPikachuSpecial1FileID ((intptr_t)0xf4)
#define llNPikachuMainFileID ((intptr_t)0xf5)
#define llYoshiMainMotionFileID ((intptr_t)0xf6)
#define llYoshiMainFileID ((intptr_t)0xf7)
#define llNYoshiMainFileID ((intptr_t)0xf8)
#define llBossMainMotionFileID ((intptr_t)0xf9)
#define llBossMainFileID ((intptr_t)0xfa)
#define llITCommonDataFileID ((intptr_t)0xfb)
#define llSCExplainMainFileID ((intptr_t)0xfc)
#define ll_253_FileID ((intptr_t)0xfd)
#define llSC1PTrainingModeFileID ((intptr_t)0xfe)
#define llGRPupupuMapFileID ((intptr_t)0xff)
#define llGRPupupuSmallMapFileID ((intptr_t)0x100)
#define llGRZebesMapFileID ((intptr_t)0x101)
#define llGRPupupuTestMapFileID ((intptr_t)0x102)
#define llGRCastleMapFileID ((intptr_t)0x103)
#define llGRInishieMapFileID ((intptr_t)0x104)
#define llGRJungleMapFileID ((intptr_t)0x105)
#define llGRSectorMapFileID ((intptr_t)0x106)
#define llGRYosterMapFileID ((intptr_t)0x107)
#define llGRYamabukiMapFileID ((intptr_t)0x108)
#define llGRHyruleMapFileID ((intptr_t)0x109)
#define llGRLastMapFileID ((intptr_t)0x10a)
#define llGRExplainMapFileID ((intptr_t)0x10b)
#define llGRZakoMapFileID ((intptr_t)0x10c)
#define llGRMetalMapFileID ((intptr_t)0x10d)
#define llGRYosterSmallMapFileID ((intptr_t)0x10e)
#define llGRBonus1MarioMapFileID ((intptr_t)0x10f)
#define llGRBonus1FoxMapFileID ((intptr_t)0x110)
#define llGRBonus1DonkeyMapFileID ((intptr_t)0x111)
#define llGRBonus1SamusMapFileID ((intptr_t)0x112)
#define llGRBonus1LuigiMapFileID ((intptr_t)0x113)
#define llGRBonus1LinkMapFileID ((intptr_t)0x114)
#define llGRBonus1YoshiMapFileID ((intptr_t)0x115)
#define llGRBonus1CaptainMapFileID ((intptr_t)0x116)
#define llGRBonus1KirbyMapFileID ((intptr_t)0x117)
#define llGRBonus1PikachuMapFileID ((intptr_t)0x118)
#define llGRBonus1PurinMapFileID ((intptr_t)0x119)
#define llGRBonus1NessMapFileID ((intptr_t)0x11a)
#define llGRBonus2MarioMapFileID ((intptr_t)0x11b)
#define llGRBonus2FoxMapFileID ((intptr_t)0x11c)
#define llGRBonus2DonkeyMapFileID ((intptr_t)0x11d)
#define llGRBonus2SamusMapFileID ((intptr_t)0x11e)
#define llGRBonus2LuigiMapFileID ((intptr_t)0x11f)
#define llGRBonus2LinkMapFileID ((intptr_t)0x120)
#define llGRBonus2YoshiMapFileID ((intptr_t)0x121)
#define llGRBonus2CaptainMapFileID ((intptr_t)0x122)
#define llGRBonus2KirbyMapFileID ((intptr_t)0x123)
#define llGRBonus2PikachuMapFileID ((intptr_t)0x124)
#define llGRBonus2PurinMapFileID ((intptr_t)0x125)
#define llGRBonus2NessMapFileID ((intptr_t)0x126)
#define llGRBonus3MapFileID ((intptr_t)0x127)
#define llMarioModelFileID ((intptr_t)0x128)
#define llMarioSpecial3FileID ((intptr_t)0x129)
#define llMarioShieldPoseFileID ((intptr_t)0x12a)
#define ll_299_FileID ((intptr_t)0x12b)
#define llMMarioModelFileID ((intptr_t)0x12c)
#define llNMarioModelFileID ((intptr_t)0x12d)
#define ll_302_FileID ((intptr_t)0x12e)
#define llNFoxModelFileID ((intptr_t)0x12f)
#define llNYoshiModelFileID ((intptr_t)0x130)
#define llNKirbyModelFileID ((intptr_t)0x131)
#define llNPurinModelFileID ((intptr_t)0x132)
#define llNPikachuModelFileID ((intptr_t)0x133)
#define llNDonkeyModelFileID ((intptr_t)0x134)
#define llNSamusModelFileID ((intptr_t)0x135)
#define llNLinkModelFileID ((intptr_t)0x136)
#define llNCaptainModelFileID ((intptr_t)0x137)
#define llNNessModelFileID ((intptr_t)0x138)
#define llFoxModelFileID ((intptr_t)0x139)
#define llFoxShieldPoseFileID ((intptr_t)0x13a)
#define ll_315_FileID ((intptr_t)0x13b)
#define llFoxSpecial4FileID ((intptr_t)0x13c)
#define llDonkeyModelFileID ((intptr_t)0x13d)
#define llDonkeyShieldPoseFileID ((intptr_t)0x13e)
#define llDkIconFileID ((intptr_t)0x13f)
#define llSamusModelFileID ((intptr_t)0x140)
#define llSamusSpecial3FileID ((intptr_t)0x141)
#define llSamusShieldPoseFileID ((intptr_t)0x142)
#define llLuigiModelFileID ((intptr_t)0x143)
#define llLinkModelFileID ((intptr_t)0x144)
#define llLinkSpecial3FileID ((intptr_t)0x145)
#define ll_326_FileID ((intptr_t)0x146)
#define llLinkShieldPoseFileID ((intptr_t)0x147)
#define llKirbyModelFileID ((intptr_t)0x148)
#define llKirbyShieldPoseFileID ((intptr_t)0x149)
#define llPurinModelFileID ((intptr_t)0x14a)
#define llPurinShieldPoseFileID ((intptr_t)0x14b)
#define llCaptainModelFileID ((intptr_t)0x14c)
#define llCaptainSpecial3FileID ((intptr_t)0x14d)
#define llCaptainShieldPoseFileID ((intptr_t)0x14e)
#define llNessModelFileID ((intptr_t)0x14f)
#define llNessSpecial3FileID ((intptr_t)0x150)
#define llNessShieldPoseFileID ((intptr_t)0x151)
#define llYoshiModelFileID ((intptr_t)0x152)
#define llYoshiSpecial3FileID ((intptr_t)0x153)
#define llYoshiShieldPoseFileID ((intptr_t)0x154)
#define llPikachuModelFileID ((intptr_t)0x155)
#define llPikachuSpecial3FileID ((intptr_t)0x156)
#define llPikachuShieldPoseFileID ((intptr_t)0x157)
#define llBossModelFileID ((intptr_t)0x158)
#define llMasterHandIconFileID ((intptr_t)0x159)
#define llFoxSpecial2FileID ((intptr_t)0x15a)
#define llPikachuSpecial2FileID ((intptr_t)0x15b)
#define llKirbySpecial2FileID ((intptr_t)0x15c)
#define llSamusSpecial2FileID ((intptr_t)0x15d)
#define llCaptainSpecial2FileID ((intptr_t)0x15e)
#define llPurinSpecial2FileID ((intptr_t)0x15f)
#define llNessSpecial2FileID ((intptr_t)0x160)
#define llLinkSpecial2FileID ((intptr_t)0x161)
#define llYoshiSpecial2FileID ((intptr_t)0x162)
#define llDonkeySpecial2FileID ((intptr_t)0x163)
#define llMarioSpecial2FileID ((intptr_t)0x164)
#define ll_357_FileID ((intptr_t)0x165)
#define ll_358_FileID ((intptr_t)0x166)
#define ll_359_FileID ((intptr_t)0x167)
#define ll_360_FileID ((intptr_t)0x168)
#define ll_361_FileID ((intptr_t)0x169)
#define ll_362_FileID ((intptr_t)0x16a)
#define ll_363_FileID ((intptr_t)0x16b)
#define ll_364_FileID ((intptr_t)0x16c)
#define ll_365_FileID ((intptr_t)0x16d)
#define ll_366_FileID ((intptr_t)0x16e)
#define ll_367_FileID ((intptr_t)0x16f)
#define ll_368_FileID ((intptr_t)0x170)
#define ll_369_FileID ((intptr_t)0x171)
#define ll_370_FileID ((intptr_t)0x172)
#define ll_371_FileID ((intptr_t)0x173)
#define ll_372_FileID ((intptr_t)0x174)
#define ll_373_FileID ((intptr_t)0x175)
#define ll_374_FileID ((intptr_t)0x176)
#define ll_375_FileID ((intptr_t)0x177)
#define ll_376_FileID ((intptr_t)0x178)
#define ll_377_FileID ((intptr_t)0x179)
#define ll_378_FileID ((intptr_t)0x17a)
#define ll_379_FileID ((intptr_t)0x17b)
#define ll_380_FileID ((intptr_t)0x17c)
#define ll_381_FileID ((intptr_t)0x17d)
#define ll_382_FileID ((intptr_t)0x17e)
#define ll_383_FileID ((intptr_t)0x17f)
#define ll_384_FileID ((intptr_t)0x180)
#define ll_385_FileID ((intptr_t)0x181)
#define ll_386_FileID ((intptr_t)0x182)
#define ll_387_FileID ((intptr_t)0x183)
#define ll_388_FileID ((intptr_t)0x184)
#define ll_389_FileID ((intptr_t)0x185)
#define ll_390_FileID ((intptr_t)0x186)
#define ll_391_FileID ((intptr_t)0x187)
#define ll_392_FileID ((intptr_t)0x188)
#define ll_393_FileID ((intptr_t)0x189)
#define ll_394_FileID ((intptr_t)0x18a)
#define ll_395_FileID ((intptr_t)0x18b)
#define ll_396_FileID ((intptr_t)0x18c)
#define ll_397_FileID ((intptr_t)0x18d)
#define ll_398_FileID ((intptr_t)0x18e)
#define ll_399_FileID ((intptr_t)0x18f)
#define ll_400_FileID ((intptr_t)0x190)
#define ll_401_FileID ((intptr_t)0x191)
#define ll_402_FileID ((intptr_t)0x192)
#define ll_403_FileID ((intptr_t)0x193)
#define ll_404_FileID ((intptr_t)0x194)
#define ll_405_FileID ((intptr_t)0x195)
#define ll_406_FileID ((intptr_t)0x196)
#define ll_407_FileID ((intptr_t)0x197)
#define ll_408_FileID ((intptr_t)0x198)
#define ll_409_FileID ((intptr_t)0x199)
#define ll_410_FileID ((intptr_t)0x19a)
#define ll_411_FileID ((intptr_t)0x19b)
#define ll_412_FileID ((intptr_t)0x19c)
#define ll_413_FileID ((intptr_t)0x19d)
#define ll_414_FileID ((intptr_t)0x19e)
#define ll_415_FileID ((intptr_t)0x19f)
#define ll_416_FileID ((intptr_t)0x1a0)
#define ll_417_FileID ((intptr_t)0x1a1)
#define ll_418_FileID ((intptr_t)0x1a2)
#define ll_419_FileID ((intptr_t)0x1a3)
#define ll_420_FileID ((intptr_t)0x1a4)
#define ll_421_FileID ((intptr_t)0x1a5)
#define ll_422_FileID ((intptr_t)0x1a6)
#define ll_423_FileID ((intptr_t)0x1a7)
#define ll_424_FileID ((intptr_t)0x1a8)
#define ll_425_FileID ((intptr_t)0x1a9)
#define ll_426_FileID ((intptr_t)0x1aa)
#define ll_427_FileID ((intptr_t)0x1ab)
#define ll_428_FileID ((intptr_t)0x1ac)
#define ll_429_FileID ((intptr_t)0x1ad)
#define ll_430_FileID ((intptr_t)0x1ae)
#define ll_431_FileID ((intptr_t)0x1af)
#define ll_432_FileID ((intptr_t)0x1b0)
#define ll_433_FileID ((intptr_t)0x1b1)
#define ll_434_FileID ((intptr_t)0x1b2)
#define ll_435_FileID ((intptr_t)0x1b3)
#define ll_436_FileID ((intptr_t)0x1b4)
#define ll_437_FileID ((intptr_t)0x1b5)
#define ll_438_FileID ((intptr_t)0x1b6)
#define ll_439_FileID ((intptr_t)0x1b7)
#define ll_440_FileID ((intptr_t)0x1b8)
#define ll_441_FileID ((intptr_t)0x1b9)
#define ll_442_FileID ((intptr_t)0x1ba)
#define ll_443_FileID ((intptr_t)0x1bb)
#define ll_444_FileID ((intptr_t)0x1bc)
#define ll_445_FileID ((intptr_t)0x1bd)
#define ll_446_FileID ((intptr_t)0x1be)
#define ll_447_FileID ((intptr_t)0x1bf)
#define ll_448_FileID ((intptr_t)0x1c0)
#define ll_449_FileID ((intptr_t)0x1c1)
#define ll_450_FileID ((intptr_t)0x1c2)
#define ll_451_FileID ((intptr_t)0x1c3)
#define ll_452_FileID ((intptr_t)0x1c4)
#define ll_453_FileID ((intptr_t)0x1c5)
#define ll_454_FileID ((intptr_t)0x1c6)
#define ll_455_FileID ((intptr_t)0x1c7)
#define ll_456_FileID ((intptr_t)0x1c8)
#define ll_457_FileID ((intptr_t)0x1c9)
#define ll_458_FileID ((intptr_t)0x1ca)
#define ll_459_FileID ((intptr_t)0x1cb)
#define ll_460_FileID ((intptr_t)0x1cc)
#define ll_461_FileID ((intptr_t)0x1cd)
#define ll_462_FileID ((intptr_t)0x1ce)
#define ll_463_FileID ((intptr_t)0x1cf)
#define ll_464_FileID ((intptr_t)0x1d0)
#define ll_465_FileID ((intptr_t)0x1d1)
#define ll_466_FileID ((intptr_t)0x1d2)
#define ll_467_FileID ((intptr_t)0x1d3)
#define ll_468_FileID ((intptr_t)0x1d4)
#define ll_469_FileID ((intptr_t)0x1d5)
#define ll_470_FileID ((intptr_t)0x1d6)
#define ll_471_FileID ((intptr_t)0x1d7)
#define ll_472_FileID ((intptr_t)0x1d8)
#define ll_473_FileID ((intptr_t)0x1d9)
#define ll_474_FileID ((intptr_t)0x1da)
#define ll_475_FileID ((intptr_t)0x1db)
#define ll_476_FileID ((intptr_t)0x1dc)
#define ll_477_FileID ((intptr_t)0x1dd)
#define ll_478_FileID ((intptr_t)0x1de)
#define ll_479_FileID ((intptr_t)0x1df)
#define ll_480_FileID ((intptr_t)0x1e0)
#define ll_481_FileID ((intptr_t)0x1e1)
#define ll_482_FileID ((intptr_t)0x1e2)
#define ll_483_FileID ((intptr_t)0x1e3)
#define ll_484_FileID ((intptr_t)0x1e4)
#define ll_485_FileID ((intptr_t)0x1e5)
#define ll_486_FileID ((intptr_t)0x1e6)
#define ll_487_FileID ((intptr_t)0x1e7)
#define ll_488_FileID ((intptr_t)0x1e8)
#define ll_489_FileID ((intptr_t)0x1e9)
#define ll_490_FileID ((intptr_t)0x1ea)
#define ll_491_FileID ((intptr_t)0x1eb)
#define ll_492_FileID ((intptr_t)0x1ec)
#define ll_493_FileID ((intptr_t)0x1ed)
#define ll_494_FileID ((intptr_t)0x1ee)
#define ll_495_FileID ((intptr_t)0x1ef)
#define ll_496_FileID ((intptr_t)0x1f0)
#define ll_497_FileID ((intptr_t)0x1f1)
#define ll_498_FileID ((intptr_t)0x1f2)
#define llFTMarioAnimWaitFileID ((intptr_t)0x1f3)
#define ll_500_FileID ((intptr_t)0x1f4)
#define ll_501_FileID ((intptr_t)0x1f5)
#define ll_502_FileID ((intptr_t)0x1f6)
#define ll_503_FileID ((intptr_t)0x1f7)
#define ll_504_FileID ((intptr_t)0x1f8)
#define ll_505_FileID ((intptr_t)0x1f9)
#define ll_506_FileID ((intptr_t)0x1fa)
#define ll_507_FileID ((intptr_t)0x1fb)
#define ll_508_FileID ((intptr_t)0x1fc)
#define ll_509_FileID ((intptr_t)0x1fd)
#define ll_510_FileID ((intptr_t)0x1fe)
#define ll_511_FileID ((intptr_t)0x1ff)
#define ll_512_FileID ((intptr_t)0x200)
#define ll_513_FileID ((intptr_t)0x201)
#define ll_514_FileID ((intptr_t)0x202)
#define ll_515_FileID ((intptr_t)0x203)
#define ll_516_FileID ((intptr_t)0x204)
#define ll_517_FileID ((intptr_t)0x205)
#define ll_518_FileID ((intptr_t)0x206)
#define ll_519_FileID ((intptr_t)0x207)
#define ll_520_FileID ((intptr_t)0x208)
#define ll_521_FileID ((intptr_t)0x209)
#define ll_522_FileID ((intptr_t)0x20a)
#define ll_523_FileID ((intptr_t)0x20b)
#define ll_524_FileID ((intptr_t)0x20c)
#define ll_525_FileID ((intptr_t)0x20d)
#define ll_526_FileID ((intptr_t)0x20e)
#define ll_527_FileID ((intptr_t)0x20f)
#define ll_528_FileID ((intptr_t)0x210)
#define ll_529_FileID ((intptr_t)0x211)
#define ll_530_FileID ((intptr_t)0x212)
#define ll_531_FileID ((intptr_t)0x213)
#define ll_532_FileID ((intptr_t)0x214)
#define ll_533_FileID ((intptr_t)0x215)
#define ll_534_FileID ((intptr_t)0x216)
#define ll_535_FileID ((intptr_t)0x217)
#define ll_536_FileID ((intptr_t)0x218)
#define ll_537_FileID ((intptr_t)0x219)
#define ll_538_FileID ((intptr_t)0x21a)
#define ll_539_FileID ((intptr_t)0x21b)
#define ll_540_FileID ((intptr_t)0x21c)
#define ll_541_FileID ((intptr_t)0x21d)
#define ll_542_FileID ((intptr_t)0x21e)
#define llFTMarioAnimDownBounceDFileID ((intptr_t)0x21f)
#define ll_544_FileID ((intptr_t)0x220)
#define llFTMarioAnimDownStandDFileID ((intptr_t)0x221)
#define ll_546_FileID ((intptr_t)0x222)
#define ll_547_FileID ((intptr_t)0x223)
#define ll_548_FileID ((intptr_t)0x224)
#define ll_549_FileID ((intptr_t)0x225)
#define ll_550_FileID ((intptr_t)0x226)
#define ll_551_FileID ((intptr_t)0x227)
#define ll_552_FileID ((intptr_t)0x228)
#define ll_553_FileID ((intptr_t)0x229)
#define ll_554_FileID ((intptr_t)0x22a)
#define ll_555_FileID ((intptr_t)0x22b)
#define ll_556_FileID ((intptr_t)0x22c)
#define ll_557_FileID ((intptr_t)0x22d)
#define ll_558_FileID ((intptr_t)0x22e)
#define ll_559_FileID ((intptr_t)0x22f)
#define ll_560_FileID ((intptr_t)0x230)
#define ll_561_FileID ((intptr_t)0x231)
#define ll_562_FileID ((intptr_t)0x232)
#define ll_563_FileID ((intptr_t)0x233)
#define ll_564_FileID ((intptr_t)0x234)
#define ll_565_FileID ((intptr_t)0x235)
#define ll_566_FileID ((intptr_t)0x236)
#define ll_567_FileID ((intptr_t)0x237)
#define ll_568_FileID ((intptr_t)0x238)
#define ll_569_FileID ((intptr_t)0x239)
#define ll_570_FileID ((intptr_t)0x23a)
#define ll_571_FileID ((intptr_t)0x23b)
#define ll_572_FileID ((intptr_t)0x23c)
#define ll_573_FileID ((intptr_t)0x23d)
#define ll_574_FileID ((intptr_t)0x23e)
#define ll_575_FileID ((intptr_t)0x23f)
#define ll_576_FileID ((intptr_t)0x240)
#define ll_577_FileID ((intptr_t)0x241)
#define ll_578_FileID ((intptr_t)0x242)
#define ll_579_FileID ((intptr_t)0x243)
#define ll_580_FileID ((intptr_t)0x244)
#define ll_581_FileID ((intptr_t)0x245)
#define ll_582_FileID ((intptr_t)0x246)
#define ll_583_FileID ((intptr_t)0x247)
#define ll_584_FileID ((intptr_t)0x248)
#define ll_585_FileID ((intptr_t)0x249)
#define ll_586_FileID ((intptr_t)0x24a)
#define ll_587_FileID ((intptr_t)0x24b)
#define ll_588_FileID ((intptr_t)0x24c)
#define ll_589_FileID ((intptr_t)0x24d)
#define ll_590_FileID ((intptr_t)0x24e)
#define ll_591_FileID ((intptr_t)0x24f)
#define ll_592_FileID ((intptr_t)0x250)
#define ll_593_FileID ((intptr_t)0x251)
#define ll_594_FileID ((intptr_t)0x252)
#define ll_595_FileID ((intptr_t)0x253)
#define ll_596_FileID ((intptr_t)0x254)
#define ll_597_FileID ((intptr_t)0x255)
#define ll_598_FileID ((intptr_t)0x256)
#define ll_599_FileID ((intptr_t)0x257)
#define ll_600_FileID ((intptr_t)0x258)
#define ll_601_FileID ((intptr_t)0x259)
#define ll_602_FileID ((intptr_t)0x25a)
#define ll_603_FileID ((intptr_t)0x25b)
#define ll_604_FileID ((intptr_t)0x25c)
#define ll_605_FileID ((intptr_t)0x25d)
#define ll_606_FileID ((intptr_t)0x25e)
#define ll_607_FileID ((intptr_t)0x25f)
#define ll_608_FileID ((intptr_t)0x260)
#define ll_609_FileID ((intptr_t)0x261)
#define ll_610_FileID ((intptr_t)0x262)
#define ll_611_FileID ((intptr_t)0x263)
#define ll_612_FileID ((intptr_t)0x264)
#define ll_613_FileID ((intptr_t)0x265)
#define ll_614_FileID ((intptr_t)0x266)
#define ll_615_FileID ((intptr_t)0x267)
#define ll_616_FileID ((intptr_t)0x268)
#define ll_617_FileID ((intptr_t)0x269)
#define ll_618_FileID ((intptr_t)0x26a)
#define ll_619_FileID ((intptr_t)0x26b)
#define ll_620_FileID ((intptr_t)0x26c)
#define ll_621_FileID ((intptr_t)0x26d)
#define ll_622_FileID ((intptr_t)0x26e)
#define ll_623_FileID ((intptr_t)0x26f)
#define ll_624_FileID ((intptr_t)0x270)
#define ll_625_FileID ((intptr_t)0x271)
#define ll_626_FileID ((intptr_t)0x272)
#define ll_627_FileID ((intptr_t)0x273)
#define ll_628_FileID ((intptr_t)0x274)
#define ll_629_FileID ((intptr_t)0x275)
#define ll_630_FileID ((intptr_t)0x276)
#define ll_631_FileID ((intptr_t)0x277)
#define ll_632_FileID ((intptr_t)0x278)
#define ll_633_FileID ((intptr_t)0x279)
#define ll_634_FileID ((intptr_t)0x27a)
#define ll_635_FileID ((intptr_t)0x27b)
#define ll_636_FileID ((intptr_t)0x27c)
#define ll_637_FileID ((intptr_t)0x27d)
#define ll_638_FileID ((intptr_t)0x27e)
#define ll_639_FileID ((intptr_t)0x27f)
#define ll_640_FileID ((intptr_t)0x280)
#define ll_641_FileID ((intptr_t)0x281)
#define ll_642_FileID ((intptr_t)0x282)
#define ll_643_FileID ((intptr_t)0x283)
#define ll_644_FileID ((intptr_t)0x284)
#define ll_645_FileID ((intptr_t)0x285)
#define ll_646_FileID ((intptr_t)0x286)
#define ll_647_FileID ((intptr_t)0x287)
#define ll_648_FileID ((intptr_t)0x288)
#define ll_649_FileID ((intptr_t)0x289)
#define ll_650_FileID ((intptr_t)0x28a)
#define ll_651_FileID ((intptr_t)0x28b)
#define ll_652_FileID ((intptr_t)0x28c)
#define ll_653_FileID ((intptr_t)0x28d)
#define ll_654_FileID ((intptr_t)0x28e)
#define ll_655_FileID ((intptr_t)0x28f)
#define ll_656_FileID ((intptr_t)0x290)
#define ll_657_FileID ((intptr_t)0x291)
#define ll_658_FileID ((intptr_t)0x292)
#define ll_659_FileID ((intptr_t)0x293)
#define ll_660_FileID ((intptr_t)0x294)
#define ll_661_FileID ((intptr_t)0x295)
#define ll_662_FileID ((intptr_t)0x296)
#define ll_663_FileID ((intptr_t)0x297)
#define ll_664_FileID ((intptr_t)0x298)
#define ll_665_FileID ((intptr_t)0x299)
#define ll_666_FileID ((intptr_t)0x29a)
#define ll_667_FileID ((intptr_t)0x29b)
#define ll_668_FileID ((intptr_t)0x29c)
#define ll_669_FileID ((intptr_t)0x29d)
#define ll_670_FileID ((intptr_t)0x29e)
#define ll_671_FileID ((intptr_t)0x29f)
#define ll_672_FileID ((intptr_t)0x2a0)
#define ll_673_FileID ((intptr_t)0x2a1)
#define ll_674_FileID ((intptr_t)0x2a2)
#define ll_675_FileID ((intptr_t)0x2a3)
#define ll_676_FileID ((intptr_t)0x2a4)
#define ll_677_FileID ((intptr_t)0x2a5)
#define ll_678_FileID ((intptr_t)0x2a6)
#define ll_679_FileID ((intptr_t)0x2a7)
#define ll_680_FileID ((intptr_t)0x2a8)
#define ll_681_FileID ((intptr_t)0x2a9)
#define ll_682_FileID ((intptr_t)0x2aa)
#define ll_683_FileID ((intptr_t)0x2ab)
#define ll_684_FileID ((intptr_t)0x2ac)
#define ll_685_FileID ((intptr_t)0x2ad)
#define ll_686_FileID ((intptr_t)0x2ae)
#define ll_687_FileID ((intptr_t)0x2af)
#define ll_688_FileID ((intptr_t)0x2b0)
#define ll_689_FileID ((intptr_t)0x2b1)
#define ll_690_FileID ((intptr_t)0x2b2)
#define ll_691_FileID ((intptr_t)0x2b3)
#define ll_692_FileID ((intptr_t)0x2b4)
#define ll_693_FileID ((intptr_t)0x2b5)
#define ll_694_FileID ((intptr_t)0x2b6)
#define ll_695_FileID ((intptr_t)0x2b7)
#define ll_696_FileID ((intptr_t)0x2b8)
#define ll_697_FileID ((intptr_t)0x2b9)
#define ll_698_FileID ((intptr_t)0x2ba)
#define ll_699_FileID ((intptr_t)0x2bb)
#define ll_700_FileID ((intptr_t)0x2bc)
#define ll_701_FileID ((intptr_t)0x2bd)
#define ll_702_FileID ((intptr_t)0x2be)
#define ll_703_FileID ((intptr_t)0x2bf)
#define ll_704_FileID ((intptr_t)0x2c0)
#define ll_705_FileID ((intptr_t)0x2c1)
#define ll_706_FileID ((intptr_t)0x2c2)
#define ll_707_FileID ((intptr_t)0x2c3)
#define ll_708_FileID ((intptr_t)0x2c4)
#define ll_709_FileID ((intptr_t)0x2c5)
#define ll_710_FileID ((intptr_t)0x2c6)
#define ll_711_FileID ((intptr_t)0x2c7)
#define ll_712_FileID ((intptr_t)0x2c8)
#define ll_713_FileID ((intptr_t)0x2c9)
#define ll_714_FileID ((intptr_t)0x2ca)
#define ll_715_FileID ((intptr_t)0x2cb)
#define ll_716_FileID ((intptr_t)0x2cc)
#define ll_717_FileID ((intptr_t)0x2cd)
#define ll_718_FileID ((intptr_t)0x2ce)
#define ll_719_FileID ((intptr_t)0x2cf)
#define ll_720_FileID ((intptr_t)0x2d0)
#define ll_721_FileID ((intptr_t)0x2d1)
#define ll_722_FileID ((intptr_t)0x2d2)
#define ll_723_FileID ((intptr_t)0x2d3)
#define ll_724_FileID ((intptr_t)0x2d4)
#define ll_725_FileID ((intptr_t)0x2d5)
#define ll_726_FileID ((intptr_t)0x2d6)
#define ll_727_FileID ((intptr_t)0x2d7)
#define ll_728_FileID ((intptr_t)0x2d8)
#define ll_729_FileID ((intptr_t)0x2d9)
#define ll_730_FileID ((intptr_t)0x2da)
#define ll_731_FileID ((intptr_t)0x2db)
#define ll_732_FileID ((intptr_t)0x2dc)
#define ll_733_FileID ((intptr_t)0x2dd)
#define ll_734_FileID ((intptr_t)0x2de)
#define ll_735_FileID ((intptr_t)0x2df)
#define ll_736_FileID ((intptr_t)0x2e0)
#define ll_737_FileID ((intptr_t)0x2e1)
#define ll_738_FileID ((intptr_t)0x2e2)
#define ll_739_FileID ((intptr_t)0x2e3)
#define ll_740_FileID ((intptr_t)0x2e4)
#define ll_741_FileID ((intptr_t)0x2e5)
#define ll_742_FileID ((intptr_t)0x2e6)
#define ll_743_FileID ((intptr_t)0x2e7)
#define ll_744_FileID ((intptr_t)0x2e8)
#define ll_745_FileID ((intptr_t)0x2e9)
#define ll_746_FileID ((intptr_t)0x2ea)
#define ll_747_FileID ((intptr_t)0x2eb)
#define ll_748_FileID ((intptr_t)0x2ec)
#define ll_749_FileID ((intptr_t)0x2ed)
#define ll_750_FileID ((intptr_t)0x2ee)
#define ll_751_FileID ((intptr_t)0x2ef)
#define ll_752_FileID ((intptr_t)0x2f0)
#define ll_753_FileID ((intptr_t)0x2f1)
#define ll_754_FileID ((intptr_t)0x2f2)
#define ll_755_FileID ((intptr_t)0x2f3)
#define ll_756_FileID ((intptr_t)0x2f4)
#define ll_757_FileID ((intptr_t)0x2f5)
#define ll_758_FileID ((intptr_t)0x2f6)
#define ll_759_FileID ((intptr_t)0x2f7)
#define ll_760_FileID ((intptr_t)0x2f8)
#define ll_761_FileID ((intptr_t)0x2f9)
#define ll_762_FileID ((intptr_t)0x2fa)
#define ll_763_FileID ((intptr_t)0x2fb)
#define ll_764_FileID ((intptr_t)0x2fc)
#define ll_765_FileID ((intptr_t)0x2fd)
#define ll_766_FileID ((intptr_t)0x2fe)
#define ll_767_FileID ((intptr_t)0x2ff)
#define ll_768_FileID ((intptr_t)0x300)
#define ll_769_FileID ((intptr_t)0x301)
#define ll_770_FileID ((intptr_t)0x302)
#define ll_771_FileID ((intptr_t)0x303)
#define ll_772_FileID ((intptr_t)0x304)
#define ll_773_FileID ((intptr_t)0x305)
#define ll_774_FileID ((intptr_t)0x306)
#define ll_775_FileID ((intptr_t)0x307)
#define ll_776_FileID ((intptr_t)0x308)
#define ll_777_FileID ((intptr_t)0x309)
#define ll_778_FileID ((intptr_t)0x30a)
#define ll_779_FileID ((intptr_t)0x30b)
#define ll_780_FileID ((intptr_t)0x30c)
#define ll_781_FileID ((intptr_t)0x30d)
#define ll_782_FileID ((intptr_t)0x30e)
#define ll_783_FileID ((intptr_t)0x30f)
#define ll_784_FileID ((intptr_t)0x310)
#define ll_785_FileID ((intptr_t)0x311)
#define ll_786_FileID ((intptr_t)0x312)
#define ll_787_FileID ((intptr_t)0x313)
#define ll_788_FileID ((intptr_t)0x314)
#define ll_789_FileID ((intptr_t)0x315)
#define ll_790_FileID ((intptr_t)0x316)
#define ll_791_FileID ((intptr_t)0x317)
#define ll_792_FileID ((intptr_t)0x318)
#define ll_793_FileID ((intptr_t)0x319)
#define ll_794_FileID ((intptr_t)0x31a)
#define ll_795_FileID ((intptr_t)0x31b)
#define ll_796_FileID ((intptr_t)0x31c)
#define ll_797_FileID ((intptr_t)0x31d)
#define ll_798_FileID ((intptr_t)0x31e)
#define ll_799_FileID ((intptr_t)0x31f)
#define ll_800_FileID ((intptr_t)0x320)
#define ll_801_FileID ((intptr_t)0x321)
#define ll_802_FileID ((intptr_t)0x322)
#define ll_803_FileID ((intptr_t)0x323)
#define ll_804_FileID ((intptr_t)0x324)
#define ll_805_FileID ((intptr_t)0x325)
#define ll_806_FileID ((intptr_t)0x326)
#define ll_807_FileID ((intptr_t)0x327)
#define ll_808_FileID ((intptr_t)0x328)
#define ll_809_FileID ((intptr_t)0x329)
#define ll_810_FileID ((intptr_t)0x32a)
#define ll_811_FileID ((intptr_t)0x32b)
#define ll_812_FileID ((intptr_t)0x32c)
#define ll_813_FileID ((intptr_t)0x32d)
#define ll_814_FileID ((intptr_t)0x32e)
#define ll_815_FileID ((intptr_t)0x32f)
#define ll_816_FileID ((intptr_t)0x330)
#define ll_817_FileID ((intptr_t)0x331)
#define ll_818_FileID ((intptr_t)0x332)
#define ll_819_FileID ((intptr_t)0x333)
#define ll_820_FileID ((intptr_t)0x334)
#define ll_821_FileID ((intptr_t)0x335)
#define ll_822_FileID ((intptr_t)0x336)
#define ll_823_FileID ((intptr_t)0x337)
#define ll_824_FileID ((intptr_t)0x338)
#define ll_825_FileID ((intptr_t)0x339)
#define ll_826_FileID ((intptr_t)0x33a)
#define ll_827_FileID ((intptr_t)0x33b)
#define ll_828_FileID ((intptr_t)0x33c)
#define ll_829_FileID ((intptr_t)0x33d)
#define ll_830_FileID ((intptr_t)0x33e)
#define ll_831_FileID ((intptr_t)0x33f)
#define ll_832_FileID ((intptr_t)0x340)
#define ll_833_FileID ((intptr_t)0x341)
#define ll_834_FileID ((intptr_t)0x342)
#define ll_835_FileID ((intptr_t)0x343)
#define ll_836_FileID ((intptr_t)0x344)
#define ll_837_FileID ((intptr_t)0x345)
#define ll_838_FileID ((intptr_t)0x346)
#define ll_839_FileID ((intptr_t)0x347)
#define ll_840_FileID ((intptr_t)0x348)
#define ll_841_FileID ((intptr_t)0x349)
#define ll_842_FileID ((intptr_t)0x34a)
#define ll_843_FileID ((intptr_t)0x34b)
#define ll_844_FileID ((intptr_t)0x34c)
#define ll_845_FileID ((intptr_t)0x34d)
#define ll_846_FileID ((intptr_t)0x34e)
#define ll_847_FileID ((intptr_t)0x34f)
#define ll_848_FileID ((intptr_t)0x350)
#define ll_849_FileID ((intptr_t)0x351)
#define ll_850_FileID ((intptr_t)0x352)
#define ll_851_FileID ((intptr_t)0x353)
#define ll_852_FileID ((intptr_t)0x354)
#define ll_853_FileID ((intptr_t)0x355)
#define ll_854_FileID ((intptr_t)0x356)
#define ll_855_FileID ((intptr_t)0x357)
#define ll_856_FileID ((intptr_t)0x358)
#define ll_857_FileID ((intptr_t)0x359)
#define ll_858_FileID ((intptr_t)0x35a)
#define ll_859_FileID ((intptr_t)0x35b)
#define ll_860_FileID ((intptr_t)0x35c)
#define ll_861_FileID ((intptr_t)0x35d)
#define ll_862_FileID ((intptr_t)0x35e)
#define ll_863_FileID ((intptr_t)0x35f)
#define ll_864_FileID ((intptr_t)0x360)
#define ll_865_FileID ((intptr_t)0x361)
#define ll_866_FileID ((intptr_t)0x362)
#define ll_867_FileID ((intptr_t)0x363)
#define ll_868_FileID ((intptr_t)0x364)
#define ll_869_FileID ((intptr_t)0x365)
#define ll_870_FileID ((intptr_t)0x366)
#define ll_871_FileID ((intptr_t)0x367)
#define ll_872_FileID ((intptr_t)0x368)
#define ll_873_FileID ((intptr_t)0x369)
#define ll_874_FileID ((intptr_t)0x36a)
#define ll_875_FileID ((intptr_t)0x36b)
#define ll_876_FileID ((intptr_t)0x36c)
#define ll_877_FileID ((intptr_t)0x36d)
#define ll_878_FileID ((intptr_t)0x36e)
#define ll_879_FileID ((intptr_t)0x36f)
#define ll_880_FileID ((intptr_t)0x370)
#define ll_881_FileID ((intptr_t)0x371)
#define ll_882_FileID ((intptr_t)0x372)
#define ll_883_FileID ((intptr_t)0x373)
#define ll_884_FileID ((intptr_t)0x374)
#define ll_885_FileID ((intptr_t)0x375)
#define ll_886_FileID ((intptr_t)0x376)
#define ll_887_FileID ((intptr_t)0x377)
#define ll_888_FileID ((intptr_t)0x378)
#define ll_889_FileID ((intptr_t)0x379)
#define ll_890_FileID ((intptr_t)0x37a)
#define ll_891_FileID ((intptr_t)0x37b)
#define ll_892_FileID ((intptr_t)0x37c)
#define ll_893_FileID ((intptr_t)0x37d)
#define ll_894_FileID ((intptr_t)0x37e)
#define ll_895_FileID ((intptr_t)0x37f)
#define ll_896_FileID ((intptr_t)0x380)
#define ll_897_FileID ((intptr_t)0x381)
#define ll_898_FileID ((intptr_t)0x382)
#define ll_899_FileID ((intptr_t)0x383)
#define ll_900_FileID ((intptr_t)0x384)
#define ll_901_FileID ((intptr_t)0x385)
#define ll_902_FileID ((intptr_t)0x386)
#define ll_903_FileID ((intptr_t)0x387)
#define ll_904_FileID ((intptr_t)0x388)
#define ll_905_FileID ((intptr_t)0x389)
#define ll_906_FileID ((intptr_t)0x38a)
#define ll_907_FileID ((intptr_t)0x38b)
#define ll_908_FileID ((intptr_t)0x38c)
#define ll_909_FileID ((intptr_t)0x38d)
#define ll_910_FileID ((intptr_t)0x38e)
#define ll_911_FileID ((intptr_t)0x38f)
#define ll_912_FileID ((intptr_t)0x390)
#define ll_913_FileID ((intptr_t)0x391)
#define ll_914_FileID ((intptr_t)0x392)
#define ll_915_FileID ((intptr_t)0x393)
#define ll_916_FileID ((intptr_t)0x394)
#define ll_917_FileID ((intptr_t)0x395)
#define ll_918_FileID ((intptr_t)0x396)
#define ll_919_FileID ((intptr_t)0x397)
#define ll_920_FileID ((intptr_t)0x398)
#define ll_921_FileID ((intptr_t)0x399)
#define ll_922_FileID ((intptr_t)0x39a)
#define ll_923_FileID ((intptr_t)0x39b)
#define ll_924_FileID ((intptr_t)0x39c)
#define ll_925_FileID ((intptr_t)0x39d)
#define ll_926_FileID ((intptr_t)0x39e)
#define ll_927_FileID ((intptr_t)0x39f)
#define ll_928_FileID ((intptr_t)0x3a0)
#define ll_929_FileID ((intptr_t)0x3a1)
#define ll_930_FileID ((intptr_t)0x3a2)
#define ll_931_FileID ((intptr_t)0x3a3)
#define ll_932_FileID ((intptr_t)0x3a4)
#define ll_933_FileID ((intptr_t)0x3a5)
#define ll_934_FileID ((intptr_t)0x3a6)
#define ll_935_FileID ((intptr_t)0x3a7)
#define ll_936_FileID ((intptr_t)0x3a8)
#define ll_937_FileID ((intptr_t)0x3a9)
#define ll_938_FileID ((intptr_t)0x3aa)
#define ll_939_FileID ((intptr_t)0x3ab)
#define ll_940_FileID ((intptr_t)0x3ac)
#define ll_941_FileID ((intptr_t)0x3ad)
#define ll_942_FileID ((intptr_t)0x3ae)
#define ll_943_FileID ((intptr_t)0x3af)
#define ll_944_FileID ((intptr_t)0x3b0)
#define ll_945_FileID ((intptr_t)0x3b1)
#define ll_946_FileID ((intptr_t)0x3b2)
#define ll_947_FileID ((intptr_t)0x3b3)
#define ll_948_FileID ((intptr_t)0x3b4)
#define ll_949_FileID ((intptr_t)0x3b5)
#define ll_950_FileID ((intptr_t)0x3b6)
#define ll_951_FileID ((intptr_t)0x3b7)
#define ll_952_FileID ((intptr_t)0x3b8)
#define ll_953_FileID ((intptr_t)0x3b9)
#define ll_954_FileID ((intptr_t)0x3ba)
#define ll_955_FileID ((intptr_t)0x3bb)
#define ll_956_FileID ((intptr_t)0x3bc)
#define ll_957_FileID ((intptr_t)0x3bd)
#define ll_958_FileID ((intptr_t)0x3be)
#define ll_959_FileID ((intptr_t)0x3bf)
#define ll_960_FileID ((intptr_t)0x3c0)
#define ll_961_FileID ((intptr_t)0x3c1)
#define ll_962_FileID ((intptr_t)0x3c2)
#define ll_963_FileID ((intptr_t)0x3c3)
#define ll_964_FileID ((intptr_t)0x3c4)
#define ll_965_FileID ((intptr_t)0x3c5)
#define ll_966_FileID ((intptr_t)0x3c6)
#define ll_967_FileID ((intptr_t)0x3c7)
#define ll_968_FileID ((intptr_t)0x3c8)
#define ll_969_FileID ((intptr_t)0x3c9)
#define ll_970_FileID ((intptr_t)0x3ca)
#define ll_971_FileID ((intptr_t)0x3cb)
#define ll_972_FileID ((intptr_t)0x3cc)
#define ll_973_FileID ((intptr_t)0x3cd)
#define ll_974_FileID ((intptr_t)0x3ce)
#define ll_975_FileID ((intptr_t)0x3cf)
#define ll_976_FileID ((intptr_t)0x3d0)
#define ll_977_FileID ((intptr_t)0x3d1)
#define ll_978_FileID ((intptr_t)0x3d2)
#define ll_979_FileID ((intptr_t)0x3d3)
#define ll_980_FileID ((intptr_t)0x3d4)
#define ll_981_FileID ((intptr_t)0x3d5)
#define ll_982_FileID ((intptr_t)0x3d6)
#define ll_983_FileID ((intptr_t)0x3d7)
#define ll_984_FileID ((intptr_t)0x3d8)
#define ll_985_FileID ((intptr_t)0x3d9)
#define ll_986_FileID ((intptr_t)0x3da)
#define ll_987_FileID ((intptr_t)0x3db)
#define ll_988_FileID ((intptr_t)0x3dc)
#define ll_989_FileID ((intptr_t)0x3dd)
#define ll_990_FileID ((intptr_t)0x3de)
#define ll_991_FileID ((intptr_t)0x3df)
#define ll_992_FileID ((intptr_t)0x3e0)
#define ll_993_FileID ((intptr_t)0x3e1)
#define ll_994_FileID ((intptr_t)0x3e2)
#define ll_995_FileID ((intptr_t)0x3e3)
#define ll_996_FileID ((intptr_t)0x3e4)
#define ll_997_FileID ((intptr_t)0x3e5)
#define ll_998_FileID ((intptr_t)0x3e6)
#define ll_999_FileID ((intptr_t)0x3e7)
#define ll_1000_FileID ((intptr_t)0x3e8)
#define ll_1001_FileID ((intptr_t)0x3e9)
#define ll_1002_FileID ((intptr_t)0x3ea)
#define ll_1003_FileID ((intptr_t)0x3eb)
#define ll_1004_FileID ((intptr_t)0x3ec)
#define ll_1005_FileID ((intptr_t)0x3ed)
#define ll_1006_FileID ((intptr_t)0x3ee)
#define ll_1007_FileID ((intptr_t)0x3ef)
#define ll_1008_FileID ((intptr_t)0x3f0)
#define ll_1009_FileID ((intptr_t)0x3f1)
#define ll_1010_FileID ((intptr_t)0x3f2)
#define ll_1011_FileID ((intptr_t)0x3f3)
#define ll_1012_FileID ((intptr_t)0x3f4)
#define ll_1013_FileID ((intptr_t)0x3f5)
#define ll_1014_FileID ((intptr_t)0x3f6)
#define ll_1015_FileID ((intptr_t)0x3f7)
#define ll_1016_FileID ((intptr_t)0x3f8)
#define ll_1017_FileID ((intptr_t)0x3f9)
#define ll_1018_FileID ((intptr_t)0x3fa)
#define ll_1019_FileID ((intptr_t)0x3fb)
#define ll_1020_FileID ((intptr_t)0x3fc)
#define ll_1021_FileID ((intptr_t)0x3fd)
#define ll_1022_FileID ((intptr_t)0x3fe)
#define ll_1023_FileID ((intptr_t)0x3ff)
#define ll_1024_FileID ((intptr_t)0x400)
#define ll_1025_FileID ((intptr_t)0x401)
#define ll_1026_FileID ((intptr_t)0x402)
#define ll_1027_FileID ((intptr_t)0x403)
#define ll_1028_FileID ((intptr_t)0x404)
#define ll_1029_FileID ((intptr_t)0x405)
#define ll_1030_FileID ((intptr_t)0x406)
#define ll_1031_FileID ((intptr_t)0x407)
#define ll_1032_FileID ((intptr_t)0x408)
#define ll_1033_FileID ((intptr_t)0x409)
#define ll_1034_FileID ((intptr_t)0x40a)
#define ll_1035_FileID ((intptr_t)0x40b)
#define ll_1036_FileID ((intptr_t)0x40c)
#define ll_1037_FileID ((intptr_t)0x40d)
#define ll_1038_FileID ((intptr_t)0x40e)
#define ll_1039_FileID ((intptr_t)0x40f)
#define ll_1040_FileID ((intptr_t)0x410)
#define ll_1041_FileID ((intptr_t)0x411)
#define ll_1042_FileID ((intptr_t)0x412)
#define ll_1043_FileID ((intptr_t)0x413)
#define ll_1044_FileID ((intptr_t)0x414)
#define ll_1045_FileID ((intptr_t)0x415)
#define ll_1046_FileID ((intptr_t)0x416)
#define ll_1047_FileID ((intptr_t)0x417)
#define ll_1048_FileID ((intptr_t)0x418)
#define ll_1049_FileID ((intptr_t)0x419)
#define ll_1050_FileID ((intptr_t)0x41a)
#define ll_1051_FileID ((intptr_t)0x41b)
#define ll_1052_FileID ((intptr_t)0x41c)
#define ll_1053_FileID ((intptr_t)0x41d)
#define ll_1054_FileID ((intptr_t)0x41e)
#define ll_1055_FileID ((intptr_t)0x41f)
#define ll_1056_FileID ((intptr_t)0x420)
#define ll_1057_FileID ((intptr_t)0x421)
#define ll_1058_FileID ((intptr_t)0x422)
#define ll_1059_FileID ((intptr_t)0x423)
#define ll_1060_FileID ((intptr_t)0x424)
#define ll_1061_FileID ((intptr_t)0x425)
#define ll_1062_FileID ((intptr_t)0x426)
#define ll_1063_FileID ((intptr_t)0x427)
#define ll_1064_FileID ((intptr_t)0x428)
#define ll_1065_FileID ((intptr_t)0x429)
#define ll_1066_FileID ((intptr_t)0x42a)
#define ll_1067_FileID ((intptr_t)0x42b)
#define ll_1068_FileID ((intptr_t)0x42c)
#define ll_1069_FileID ((intptr_t)0x42d)
#define ll_1070_FileID ((intptr_t)0x42e)
#define ll_1071_FileID ((intptr_t)0x42f)
#define ll_1072_FileID ((intptr_t)0x430)
#define ll_1073_FileID ((intptr_t)0x431)
#define ll_1074_FileID ((intptr_t)0x432)
#define ll_1075_FileID ((intptr_t)0x433)
#define ll_1076_FileID ((intptr_t)0x434)
#define ll_1077_FileID ((intptr_t)0x435)
#define ll_1078_FileID ((intptr_t)0x436)
#define ll_1079_FileID ((intptr_t)0x437)
#define ll_1080_FileID ((intptr_t)0x438)
#define ll_1081_FileID ((intptr_t)0x439)
#define ll_1082_FileID ((intptr_t)0x43a)
#define ll_1083_FileID ((intptr_t)0x43b)
#define ll_1084_FileID ((intptr_t)0x43c)
#define ll_1085_FileID ((intptr_t)0x43d)
#define ll_1086_FileID ((intptr_t)0x43e)
#define ll_1087_FileID ((intptr_t)0x43f)
#define ll_1088_FileID ((intptr_t)0x440)
#define ll_1089_FileID ((intptr_t)0x441)
#define ll_1090_FileID ((intptr_t)0x442)
#define ll_1091_FileID ((intptr_t)0x443)
#define ll_1092_FileID ((intptr_t)0x444)
#define ll_1093_FileID ((intptr_t)0x445)
#define ll_1094_FileID ((intptr_t)0x446)
#define ll_1095_FileID ((intptr_t)0x447)
#define ll_1096_FileID ((intptr_t)0x448)
#define ll_1097_FileID ((intptr_t)0x449)
#define ll_1098_FileID ((intptr_t)0x44a)
#define ll_1099_FileID ((intptr_t)0x44b)
#define ll_1100_FileID ((intptr_t)0x44c)
#define ll_1101_FileID ((intptr_t)0x44d)
#define ll_1102_FileID ((intptr_t)0x44e)
#define ll_1103_FileID ((intptr_t)0x44f)
#define ll_1104_FileID ((intptr_t)0x450)
#define ll_1105_FileID ((intptr_t)0x451)
#define ll_1106_FileID ((intptr_t)0x452)
#define ll_1107_FileID ((intptr_t)0x453)
#define ll_1108_FileID ((intptr_t)0x454)
#define ll_1109_FileID ((intptr_t)0x455)
#define ll_1110_FileID ((intptr_t)0x456)
#define ll_1111_FileID ((intptr_t)0x457)
#define ll_1112_FileID ((intptr_t)0x458)
#define ll_1113_FileID ((intptr_t)0x459)
#define ll_1114_FileID ((intptr_t)0x45a)
#define ll_1115_FileID ((intptr_t)0x45b)
#define ll_1116_FileID ((intptr_t)0x45c)
#define ll_1117_FileID ((intptr_t)0x45d)
#define ll_1118_FileID ((intptr_t)0x45e)
#define ll_1119_FileID ((intptr_t)0x45f)
#define ll_1120_FileID ((intptr_t)0x460)
#define ll_1121_FileID ((intptr_t)0x461)
#define ll_1122_FileID ((intptr_t)0x462)
#define ll_1123_FileID ((intptr_t)0x463)
#define ll_1124_FileID ((intptr_t)0x464)
#define ll_1125_FileID ((intptr_t)0x465)
#define ll_1126_FileID ((intptr_t)0x466)
#define ll_1127_FileID ((intptr_t)0x467)
#define ll_1128_FileID ((intptr_t)0x468)
#define ll_1129_FileID ((intptr_t)0x469)
#define ll_1130_FileID ((intptr_t)0x46a)
#define ll_1131_FileID ((intptr_t)0x46b)
#define ll_1132_FileID ((intptr_t)0x46c)
#define ll_1133_FileID ((intptr_t)0x46d)
#define ll_1134_FileID ((intptr_t)0x46e)
#define ll_1135_FileID ((intptr_t)0x46f)
#define ll_1136_FileID ((intptr_t)0x470)
#define ll_1137_FileID ((intptr_t)0x471)
#define ll_1138_FileID ((intptr_t)0x472)
#define ll_1139_FileID ((intptr_t)0x473)
#define ll_1140_FileID ((intptr_t)0x474)
#define ll_1141_FileID ((intptr_t)0x475)
#define ll_1142_FileID ((intptr_t)0x476)
#define ll_1143_FileID ((intptr_t)0x477)
#define ll_1144_FileID ((intptr_t)0x478)
#define ll_1145_FileID ((intptr_t)0x479)
#define ll_1146_FileID ((intptr_t)0x47a)
#define ll_1147_FileID ((intptr_t)0x47b)
#define ll_1148_FileID ((intptr_t)0x47c)
#define ll_1149_FileID ((intptr_t)0x47d)
#define ll_1150_FileID ((intptr_t)0x47e)
#define ll_1151_FileID ((intptr_t)0x47f)
#define ll_1152_FileID ((intptr_t)0x480)
#define ll_1153_FileID ((intptr_t)0x481)
#define ll_1154_FileID ((intptr_t)0x482)
#define ll_1155_FileID ((intptr_t)0x483)
#define ll_1156_FileID ((intptr_t)0x484)
#define ll_1157_FileID ((intptr_t)0x485)
#define ll_1158_FileID ((intptr_t)0x486)
#define ll_1159_FileID ((intptr_t)0x487)
#define ll_1160_FileID ((intptr_t)0x488)
#define ll_1161_FileID ((intptr_t)0x489)
#define ll_1162_FileID ((intptr_t)0x48a)
#define ll_1163_FileID ((intptr_t)0x48b)
#define ll_1164_FileID ((intptr_t)0x48c)
#define ll_1165_FileID ((intptr_t)0x48d)
#define ll_1166_FileID ((intptr_t)0x48e)
#define ll_1167_FileID ((intptr_t)0x48f)
#define ll_1168_FileID ((intptr_t)0x490)
#define ll_1169_FileID ((intptr_t)0x491)
#define ll_1170_FileID ((intptr_t)0x492)
#define ll_1171_FileID ((intptr_t)0x493)
#define ll_1172_FileID ((intptr_t)0x494)
#define ll_1173_FileID ((intptr_t)0x495)
#define ll_1174_FileID ((intptr_t)0x496)
#define ll_1175_FileID ((intptr_t)0x497)
#define ll_1176_FileID ((intptr_t)0x498)
#define ll_1177_FileID ((intptr_t)0x499)
#define ll_1178_FileID ((intptr_t)0x49a)
#define ll_1179_FileID ((intptr_t)0x49b)
#define ll_1180_FileID ((intptr_t)0x49c)
#define ll_1181_FileID ((intptr_t)0x49d)
#define ll_1182_FileID ((intptr_t)0x49e)
#define ll_1183_FileID ((intptr_t)0x49f)
#define ll_1184_FileID ((intptr_t)0x4a0)
#define ll_1185_FileID ((intptr_t)0x4a1)
#define ll_1186_FileID ((intptr_t)0x4a2)
#define ll_1187_FileID ((intptr_t)0x4a3)
#define ll_1188_FileID ((intptr_t)0x4a4)
#define ll_1189_FileID ((intptr_t)0x4a5)
#define ll_1190_FileID ((intptr_t)0x4a6)
#define ll_1191_FileID ((intptr_t)0x4a7)
#define ll_1192_FileID ((intptr_t)0x4a8)
#define ll_1193_FileID ((intptr_t)0x4a9)
#define ll_1194_FileID ((intptr_t)0x4aa)
#define ll_1195_FileID ((intptr_t)0x4ab)
#define ll_1196_FileID ((intptr_t)0x4ac)
#define ll_1197_FileID ((intptr_t)0x4ad)
#define ll_1198_FileID ((intptr_t)0x4ae)
#define ll_1199_FileID ((intptr_t)0x4af)
#define ll_1200_FileID ((intptr_t)0x4b0)
#define ll_1201_FileID ((intptr_t)0x4b1)
#define ll_1202_FileID ((intptr_t)0x4b2)
#define ll_1203_FileID ((intptr_t)0x4b3)
#define ll_1204_FileID ((intptr_t)0x4b4)
#define ll_1205_FileID ((intptr_t)0x4b5)
#define ll_1206_FileID ((intptr_t)0x4b6)
#define ll_1207_FileID ((intptr_t)0x4b7)
#define ll_1208_FileID ((intptr_t)0x4b8)
#define ll_1209_FileID ((intptr_t)0x4b9)
#define ll_1210_FileID ((intptr_t)0x4ba)
#define ll_1211_FileID ((intptr_t)0x4bb)
#define ll_1212_FileID ((intptr_t)0x4bc)
#define ll_1213_FileID ((intptr_t)0x4bd)
#define ll_1214_FileID ((intptr_t)0x4be)
#define ll_1215_FileID ((intptr_t)0x4bf)
#define ll_1216_FileID ((intptr_t)0x4c0)
#define ll_1217_FileID ((intptr_t)0x4c1)
#define ll_1218_FileID ((intptr_t)0x4c2)
#define ll_1219_FileID ((intptr_t)0x4c3)
#define ll_1220_FileID ((intptr_t)0x4c4)
#define ll_1221_FileID ((intptr_t)0x4c5)
#define ll_1222_FileID ((intptr_t)0x4c6)
#define ll_1223_FileID ((intptr_t)0x4c7)
#define ll_1224_FileID ((intptr_t)0x4c8)
#define ll_1225_FileID ((intptr_t)0x4c9)
#define ll_1226_FileID ((intptr_t)0x4ca)
#define ll_1227_FileID ((intptr_t)0x4cb)
#define ll_1228_FileID ((intptr_t)0x4cc)
#define ll_1229_FileID ((intptr_t)0x4cd)
#define ll_1230_FileID ((intptr_t)0x4ce)
#define ll_1231_FileID ((intptr_t)0x4cf)
#define ll_1232_FileID ((intptr_t)0x4d0)
#define ll_1233_FileID ((intptr_t)0x4d1)
#define ll_1234_FileID ((intptr_t)0x4d2)
#define ll_1235_FileID ((intptr_t)0x4d3)
#define ll_1236_FileID ((intptr_t)0x4d4)
#define ll_1237_FileID ((intptr_t)0x4d5)
#define ll_1238_FileID ((intptr_t)0x4d6)
#define ll_1239_FileID ((intptr_t)0x4d7)
#define ll_1240_FileID ((intptr_t)0x4d8)
#define ll_1241_FileID ((intptr_t)0x4d9)
#define ll_1242_FileID ((intptr_t)0x4da)
#define ll_1243_FileID ((intptr_t)0x4db)
#define ll_1244_FileID ((intptr_t)0x4dc)
#define ll_1245_FileID ((intptr_t)0x4dd)
#define ll_1246_FileID ((intptr_t)0x4de)
#define ll_1247_FileID ((intptr_t)0x4df)
#define ll_1248_FileID ((intptr_t)0x4e0)
#define ll_1249_FileID ((intptr_t)0x4e1)
#define ll_1250_FileID ((intptr_t)0x4e2)
#define ll_1251_FileID ((intptr_t)0x4e3)
#define ll_1252_FileID ((intptr_t)0x4e4)
#define ll_1253_FileID ((intptr_t)0x4e5)
#define ll_1254_FileID ((intptr_t)0x4e6)
#define ll_1255_FileID ((intptr_t)0x4e7)
#define ll_1256_FileID ((intptr_t)0x4e8)
#define ll_1257_FileID ((intptr_t)0x4e9)
#define ll_1258_FileID ((intptr_t)0x4ea)
#define ll_1259_FileID ((intptr_t)0x4eb)
#define ll_1260_FileID ((intptr_t)0x4ec)
#define ll_1261_FileID ((intptr_t)0x4ed)
#define ll_1262_FileID ((intptr_t)0x4ee)
#define ll_1263_FileID ((intptr_t)0x4ef)
#define ll_1264_FileID ((intptr_t)0x4f0)
#define ll_1265_FileID ((intptr_t)0x4f1)
#define ll_1266_FileID ((intptr_t)0x4f2)
#define ll_1267_FileID ((intptr_t)0x4f3)
#define ll_1268_FileID ((intptr_t)0x4f4)
#define ll_1269_FileID ((intptr_t)0x4f5)
#define ll_1270_FileID ((intptr_t)0x4f6)
#define ll_1271_FileID ((intptr_t)0x4f7)
#define ll_1272_FileID ((intptr_t)0x4f8)
#define ll_1273_FileID ((intptr_t)0x4f9)
#define ll_1274_FileID ((intptr_t)0x4fa)
#define ll_1275_FileID ((intptr_t)0x4fb)
#define ll_1276_FileID ((intptr_t)0x4fc)
#define ll_1277_FileID ((intptr_t)0x4fd)
#define ll_1278_FileID ((intptr_t)0x4fe)
#define ll_1279_FileID ((intptr_t)0x4ff)
#define ll_1280_FileID ((intptr_t)0x500)
#define ll_1281_FileID ((intptr_t)0x501)
#define ll_1282_FileID ((intptr_t)0x502)
#define ll_1283_FileID ((intptr_t)0x503)
#define ll_1284_FileID ((intptr_t)0x504)
#define ll_1285_FileID ((intptr_t)0x505)
#define ll_1286_FileID ((intptr_t)0x506)
#define ll_1287_FileID ((intptr_t)0x507)
#define ll_1288_FileID ((intptr_t)0x508)
#define ll_1289_FileID ((intptr_t)0x509)
#define ll_1290_FileID ((intptr_t)0x50a)
#define ll_1291_FileID ((intptr_t)0x50b)
#define ll_1292_FileID ((intptr_t)0x50c)
#define ll_1293_FileID ((intptr_t)0x50d)
#define ll_1294_FileID ((intptr_t)0x50e)
#define ll_1295_FileID ((intptr_t)0x50f)
#define ll_1296_FileID ((intptr_t)0x510)
#define ll_1297_FileID ((intptr_t)0x511)
#define ll_1298_FileID ((intptr_t)0x512)
#define ll_1299_FileID ((intptr_t)0x513)
#define ll_1300_FileID ((intptr_t)0x514)
#define ll_1301_FileID ((intptr_t)0x515)
#define ll_1302_FileID ((intptr_t)0x516)
#define ll_1303_FileID ((intptr_t)0x517)
#define ll_1304_FileID ((intptr_t)0x518)
#define ll_1305_FileID ((intptr_t)0x519)
#define ll_1306_FileID ((intptr_t)0x51a)
#define ll_1307_FileID ((intptr_t)0x51b)
#define ll_1308_FileID ((intptr_t)0x51c)
#define ll_1309_FileID ((intptr_t)0x51d)
#define ll_1310_FileID ((intptr_t)0x51e)
#define ll_1311_FileID ((intptr_t)0x51f)
#define ll_1312_FileID ((intptr_t)0x520)
#define ll_1313_FileID ((intptr_t)0x521)
#define ll_1314_FileID ((intptr_t)0x522)
#define ll_1315_FileID ((intptr_t)0x523)
#define ll_1316_FileID ((intptr_t)0x524)
#define ll_1317_FileID ((intptr_t)0x525)
#define ll_1318_FileID ((intptr_t)0x526)
#define ll_1319_FileID ((intptr_t)0x527)
#define ll_1320_FileID ((intptr_t)0x528)
#define ll_1321_FileID ((intptr_t)0x529)
#define ll_1322_FileID ((intptr_t)0x52a)
#define ll_1323_FileID ((intptr_t)0x52b)
#define ll_1324_FileID ((intptr_t)0x52c)
#define ll_1325_FileID ((intptr_t)0x52d)
#define ll_1326_FileID ((intptr_t)0x52e)
#define ll_1327_FileID ((intptr_t)0x52f)
#define ll_1328_FileID ((intptr_t)0x530)
#define ll_1329_FileID ((intptr_t)0x531)
#define ll_1330_FileID ((intptr_t)0x532)
#define ll_1331_FileID ((intptr_t)0x533)
#define ll_1332_FileID ((intptr_t)0x534)
#define ll_1333_FileID ((intptr_t)0x535)
#define ll_1334_FileID ((intptr_t)0x536)
#define ll_1335_FileID ((intptr_t)0x537)
#define ll_1336_FileID ((intptr_t)0x538)
#define ll_1337_FileID ((intptr_t)0x539)
#define ll_1338_FileID ((intptr_t)0x53a)
#define ll_1339_FileID ((intptr_t)0x53b)
#define ll_1340_FileID ((intptr_t)0x53c)
#define ll_1341_FileID ((intptr_t)0x53d)
#define ll_1342_FileID ((intptr_t)0x53e)
#define ll_1343_FileID ((intptr_t)0x53f)
#define ll_1344_FileID ((intptr_t)0x540)
#define ll_1345_FileID ((intptr_t)0x541)
#define ll_1346_FileID ((intptr_t)0x542)
#define ll_1347_FileID ((intptr_t)0x543)
#define ll_1348_FileID ((intptr_t)0x544)
#define ll_1349_FileID ((intptr_t)0x545)
#define ll_1350_FileID ((intptr_t)0x546)
#define ll_1351_FileID ((intptr_t)0x547)
#define ll_1352_FileID ((intptr_t)0x548)
#define ll_1353_FileID ((intptr_t)0x549)
#define ll_1354_FileID ((intptr_t)0x54a)
#define ll_1355_FileID ((intptr_t)0x54b)
#define ll_1356_FileID ((intptr_t)0x54c)
#define ll_1357_FileID ((intptr_t)0x54d)
#define ll_1358_FileID ((intptr_t)0x54e)
#define ll_1359_FileID ((intptr_t)0x54f)
#define ll_1360_FileID ((intptr_t)0x550)
#define ll_1361_FileID ((intptr_t)0x551)
#define ll_1362_FileID ((intptr_t)0x552)
#define ll_1363_FileID ((intptr_t)0x553)
#define ll_1364_FileID ((intptr_t)0x554)
#define ll_1365_FileID ((intptr_t)0x555)
#define ll_1366_FileID ((intptr_t)0x556)
#define ll_1367_FileID ((intptr_t)0x557)
#define ll_1368_FileID ((intptr_t)0x558)
#define ll_1369_FileID ((intptr_t)0x559)
#define ll_1370_FileID ((intptr_t)0x55a)
#define ll_1371_FileID ((intptr_t)0x55b)
#define ll_1372_FileID ((intptr_t)0x55c)
#define ll_1373_FileID ((intptr_t)0x55d)
#define ll_1374_FileID ((intptr_t)0x55e)
#define ll_1375_FileID ((intptr_t)0x55f)
#define ll_1376_FileID ((intptr_t)0x560)
#define ll_1377_FileID ((intptr_t)0x561)
#define ll_1378_FileID ((intptr_t)0x562)
#define ll_1379_FileID ((intptr_t)0x563)
#define ll_1380_FileID ((intptr_t)0x564)
#define ll_1381_FileID ((intptr_t)0x565)
#define ll_1382_FileID ((intptr_t)0x566)
#define ll_1383_FileID ((intptr_t)0x567)
#define ll_1384_FileID ((intptr_t)0x568)
#define ll_1385_FileID ((intptr_t)0x569)
#define ll_1386_FileID ((intptr_t)0x56a)
#define ll_1387_FileID ((intptr_t)0x56b)
#define ll_1388_FileID ((intptr_t)0x56c)
#define ll_1389_FileID ((intptr_t)0x56d)
#define ll_1390_FileID ((intptr_t)0x56e)
#define ll_1391_FileID ((intptr_t)0x56f)
#define ll_1392_FileID ((intptr_t)0x570)
#define ll_1393_FileID ((intptr_t)0x571)
#define ll_1394_FileID ((intptr_t)0x572)
#define ll_1395_FileID ((intptr_t)0x573)
#define ll_1396_FileID ((intptr_t)0x574)
#define ll_1397_FileID ((intptr_t)0x575)
#define ll_1398_FileID ((intptr_t)0x576)
#define ll_1399_FileID ((intptr_t)0x577)
#define ll_1400_FileID ((intptr_t)0x578)
#define ll_1401_FileID ((intptr_t)0x579)
#define ll_1402_FileID ((intptr_t)0x57a)
#define ll_1403_FileID ((intptr_t)0x57b)
#define ll_1404_FileID ((intptr_t)0x57c)
#define ll_1405_FileID ((intptr_t)0x57d)
#define ll_1406_FileID ((intptr_t)0x57e)
#define ll_1407_FileID ((intptr_t)0x57f)
#define ll_1408_FileID ((intptr_t)0x580)
#define ll_1409_FileID ((intptr_t)0x581)
#define ll_1410_FileID ((intptr_t)0x582)
#define ll_1411_FileID ((intptr_t)0x583)
#define ll_1412_FileID ((intptr_t)0x584)
#define ll_1413_FileID ((intptr_t)0x585)
#define ll_1414_FileID ((intptr_t)0x586)
#define ll_1415_FileID ((intptr_t)0x587)
#define ll_1416_FileID ((intptr_t)0x588)
#define ll_1417_FileID ((intptr_t)0x589)
#define ll_1418_FileID ((intptr_t)0x58a)
#define ll_1419_FileID ((intptr_t)0x58b)
#define ll_1420_FileID ((intptr_t)0x58c)
#define ll_1421_FileID ((intptr_t)0x58d)
#define ll_1422_FileID ((intptr_t)0x58e)
#define ll_1423_FileID ((intptr_t)0x58f)
#define ll_1424_FileID ((intptr_t)0x590)
#define ll_1425_FileID ((intptr_t)0x591)
#define ll_1426_FileID ((intptr_t)0x592)
#define ll_1427_FileID ((intptr_t)0x593)
#define ll_1428_FileID ((intptr_t)0x594)
#define ll_1429_FileID ((intptr_t)0x595)
#define ll_1430_FileID ((intptr_t)0x596)
#define ll_1431_FileID ((intptr_t)0x597)
#define ll_1432_FileID ((intptr_t)0x598)
#define ll_1433_FileID ((intptr_t)0x599)
#define ll_1434_FileID ((intptr_t)0x59a)
#define ll_1435_FileID ((intptr_t)0x59b)
#define ll_1436_FileID ((intptr_t)0x59c)
#define ll_1437_FileID ((intptr_t)0x59d)
#define ll_1438_FileID ((intptr_t)0x59e)
#define ll_1439_FileID ((intptr_t)0x59f)
#define ll_1440_FileID ((intptr_t)0x5a0)
#define ll_1441_FileID ((intptr_t)0x5a1)
#define ll_1442_FileID ((intptr_t)0x5a2)
#define ll_1443_FileID ((intptr_t)0x5a3)
#define ll_1444_FileID ((intptr_t)0x5a4)
#define ll_1445_FileID ((intptr_t)0x5a5)
#define ll_1446_FileID ((intptr_t)0x5a6)
#define ll_1447_FileID ((intptr_t)0x5a7)
#define ll_1448_FileID ((intptr_t)0x5a8)
#define ll_1449_FileID ((intptr_t)0x5a9)
#define ll_1450_FileID ((intptr_t)0x5aa)
#define ll_1451_FileID ((intptr_t)0x5ab)
#define ll_1452_FileID ((intptr_t)0x5ac)
#define ll_1453_FileID ((intptr_t)0x5ad)
#define ll_1454_FileID ((intptr_t)0x5ae)
#define ll_1455_FileID ((intptr_t)0x5af)
#define ll_1456_FileID ((intptr_t)0x5b0)
#define ll_1457_FileID ((intptr_t)0x5b1)
#define ll_1458_FileID ((intptr_t)0x5b2)
#define ll_1459_FileID ((intptr_t)0x5b3)
#define ll_1460_FileID ((intptr_t)0x5b4)
#define ll_1461_FileID ((intptr_t)0x5b5)
#define ll_1462_FileID ((intptr_t)0x5b6)
#define ll_1463_FileID ((intptr_t)0x5b7)
#define ll_1464_FileID ((intptr_t)0x5b8)
#define ll_1465_FileID ((intptr_t)0x5b9)
#define ll_1466_FileID ((intptr_t)0x5ba)
#define ll_1467_FileID ((intptr_t)0x5bb)
#define ll_1468_FileID ((intptr_t)0x5bc)
#define ll_1469_FileID ((intptr_t)0x5bd)
#define ll_1470_FileID ((intptr_t)0x5be)
#define ll_1471_FileID ((intptr_t)0x5bf)
#define ll_1472_FileID ((intptr_t)0x5c0)
#define ll_1473_FileID ((intptr_t)0x5c1)
#define ll_1474_FileID ((intptr_t)0x5c2)
#define ll_1475_FileID ((intptr_t)0x5c3)
#define ll_1476_FileID ((intptr_t)0x5c4)
#define ll_1477_FileID ((intptr_t)0x5c5)
#define ll_1478_FileID ((intptr_t)0x5c6)
#define ll_1479_FileID ((intptr_t)0x5c7)
#define ll_1480_FileID ((intptr_t)0x5c8)
#define ll_1481_FileID ((intptr_t)0x5c9)
#define ll_1482_FileID ((intptr_t)0x5ca)
#define ll_1483_FileID ((intptr_t)0x5cb)
#define ll_1484_FileID ((intptr_t)0x5cc)
#define ll_1485_FileID ((intptr_t)0x5cd)
#define ll_1486_FileID ((intptr_t)0x5ce)
#define ll_1487_FileID ((intptr_t)0x5cf)
#define ll_1488_FileID ((intptr_t)0x5d0)
#define ll_1489_FileID ((intptr_t)0x5d1)
#define ll_1490_FileID ((intptr_t)0x5d2)
#define ll_1491_FileID ((intptr_t)0x5d3)
#define ll_1492_FileID ((intptr_t)0x5d4)
#define ll_1493_FileID ((intptr_t)0x5d5)
#define ll_1494_FileID ((intptr_t)0x5d6)
#define ll_1495_FileID ((intptr_t)0x5d7)
#define ll_1496_FileID ((intptr_t)0x5d8)
#define ll_1497_FileID ((intptr_t)0x5d9)
#define ll_1498_FileID ((intptr_t)0x5da)
#define ll_1499_FileID ((intptr_t)0x5db)
#define ll_1500_FileID ((intptr_t)0x5dc)
#define ll_1501_FileID ((intptr_t)0x5dd)
#define ll_1502_FileID ((intptr_t)0x5de)
#define ll_1503_FileID ((intptr_t)0x5df)
#define ll_1504_FileID ((intptr_t)0x5e0)
#define ll_1505_FileID ((intptr_t)0x5e1)
#define ll_1506_FileID ((intptr_t)0x5e2)
#define ll_1507_FileID ((intptr_t)0x5e3)
#define ll_1508_FileID ((intptr_t)0x5e4)
#define ll_1509_FileID ((intptr_t)0x5e5)
#define ll_1510_FileID ((intptr_t)0x5e6)
#define ll_1511_FileID ((intptr_t)0x5e7)
#define ll_1512_FileID ((intptr_t)0x5e8)
#define ll_1513_FileID ((intptr_t)0x5e9)
#define ll_1514_FileID ((intptr_t)0x5ea)
#define ll_1515_FileID ((intptr_t)0x5eb)
#define ll_1516_FileID ((intptr_t)0x5ec)
#define ll_1517_FileID ((intptr_t)0x5ed)
#define ll_1518_FileID ((intptr_t)0x5ee)
#define ll_1519_FileID ((intptr_t)0x5ef)
#define ll_1520_FileID ((intptr_t)0x5f0)
#define ll_1521_FileID ((intptr_t)0x5f1)
#define ll_1522_FileID ((intptr_t)0x5f2)
#define ll_1523_FileID ((intptr_t)0x5f3)
#define ll_1524_FileID ((intptr_t)0x5f4)
#define ll_1525_FileID ((intptr_t)0x5f5)
#define ll_1526_FileID ((intptr_t)0x5f6)
#define ll_1527_FileID ((intptr_t)0x5f7)
#define ll_1528_FileID ((intptr_t)0x5f8)
#define ll_1529_FileID ((intptr_t)0x5f9)
#define ll_1530_FileID ((intptr_t)0x5fa)
#define ll_1531_FileID ((intptr_t)0x5fb)
#define ll_1532_FileID ((intptr_t)0x5fc)
#define ll_1533_FileID ((intptr_t)0x5fd)
#define ll_1534_FileID ((intptr_t)0x5fe)
#define ll_1535_FileID ((intptr_t)0x5ff)
#define ll_1536_FileID ((intptr_t)0x600)
#define ll_1537_FileID ((intptr_t)0x601)
#define ll_1538_FileID ((intptr_t)0x602)
#define ll_1539_FileID ((intptr_t)0x603)
#define ll_1540_FileID ((intptr_t)0x604)
#define ll_1541_FileID ((intptr_t)0x605)
#define ll_1542_FileID ((intptr_t)0x606)
#define ll_1543_FileID ((intptr_t)0x607)
#define ll_1544_FileID ((intptr_t)0x608)
#define ll_1545_FileID ((intptr_t)0x609)
#define ll_1546_FileID ((intptr_t)0x60a)
#define ll_1547_FileID ((intptr_t)0x60b)
#define ll_1548_FileID ((intptr_t)0x60c)
#define ll_1549_FileID ((intptr_t)0x60d)
#define ll_1550_FileID ((intptr_t)0x60e)
#define ll_1551_FileID ((intptr_t)0x60f)
#define ll_1552_FileID ((intptr_t)0x610)
#define ll_1553_FileID ((intptr_t)0x611)
#define ll_1554_FileID ((intptr_t)0x612)
#define ll_1555_FileID ((intptr_t)0x613)
#define ll_1556_FileID ((intptr_t)0x614)
#define ll_1557_FileID ((intptr_t)0x615)
#define ll_1558_FileID ((intptr_t)0x616)
#define ll_1559_FileID ((intptr_t)0x617)
#define ll_1560_FileID ((intptr_t)0x618)
#define ll_1561_FileID ((intptr_t)0x619)
#define ll_1562_FileID ((intptr_t)0x61a)
#define ll_1563_FileID ((intptr_t)0x61b)
#define ll_1564_FileID ((intptr_t)0x61c)
#define ll_1565_FileID ((intptr_t)0x61d)
#define ll_1566_FileID ((intptr_t)0x61e)
#define ll_1567_FileID ((intptr_t)0x61f)
#define ll_1568_FileID ((intptr_t)0x620)
#define ll_1569_FileID ((intptr_t)0x621)
#define ll_1570_FileID ((intptr_t)0x622)
#define ll_1571_FileID ((intptr_t)0x623)
#define ll_1572_FileID ((intptr_t)0x624)
#define ll_1573_FileID ((intptr_t)0x625)
#define ll_1574_FileID ((intptr_t)0x626)
#define ll_1575_FileID ((intptr_t)0x627)
#define ll_1576_FileID ((intptr_t)0x628)
#define ll_1577_FileID ((intptr_t)0x629)
#define ll_1578_FileID ((intptr_t)0x62a)
#define ll_1579_FileID ((intptr_t)0x62b)
#define ll_1580_FileID ((intptr_t)0x62c)
#define ll_1581_FileID ((intptr_t)0x62d)
#define ll_1582_FileID ((intptr_t)0x62e)
#define ll_1583_FileID ((intptr_t)0x62f)
#define ll_1584_FileID ((intptr_t)0x630)
#define ll_1585_FileID ((intptr_t)0x631)
#define ll_1586_FileID ((intptr_t)0x632)
#define ll_1587_FileID ((intptr_t)0x633)
#define ll_1588_FileID ((intptr_t)0x634)
#define ll_1589_FileID ((intptr_t)0x635)
#define ll_1590_FileID ((intptr_t)0x636)
#define ll_1591_FileID ((intptr_t)0x637)
#define ll_1592_FileID ((intptr_t)0x638)
#define ll_1593_FileID ((intptr_t)0x639)
#define ll_1594_FileID ((intptr_t)0x63a)
#define ll_1595_FileID ((intptr_t)0x63b)
#define ll_1596_FileID ((intptr_t)0x63c)
#define ll_1597_FileID ((intptr_t)0x63d)
#define ll_1598_FileID ((intptr_t)0x63e)
#define ll_1599_FileID ((intptr_t)0x63f)
#define ll_1600_FileID ((intptr_t)0x640)
#define ll_1601_FileID ((intptr_t)0x641)
#define ll_1602_FileID ((intptr_t)0x642)
#define ll_1603_FileID ((intptr_t)0x643)
#define ll_1604_FileID ((intptr_t)0x644)
#define ll_1605_FileID ((intptr_t)0x645)
#define ll_1606_FileID ((intptr_t)0x646)
#define ll_1607_FileID ((intptr_t)0x647)
#define ll_1608_FileID ((intptr_t)0x648)
#define ll_1609_FileID ((intptr_t)0x649)
#define ll_1610_FileID ((intptr_t)0x64a)
#define ll_1611_FileID ((intptr_t)0x64b)
#define ll_1612_FileID ((intptr_t)0x64c)
#define ll_1613_FileID ((intptr_t)0x64d)
#define ll_1614_FileID ((intptr_t)0x64e)
#define ll_1615_FileID ((intptr_t)0x64f)
#define ll_1616_FileID ((intptr_t)0x650)
#define ll_1617_FileID ((intptr_t)0x651)
#define ll_1618_FileID ((intptr_t)0x652)
#define ll_1619_FileID ((intptr_t)0x653)
#define ll_1620_FileID ((intptr_t)0x654)
#define ll_1621_FileID ((intptr_t)0x655)
#define ll_1622_FileID ((intptr_t)0x656)
#define ll_1623_FileID ((intptr_t)0x657)
#define ll_1624_FileID ((intptr_t)0x658)
#define ll_1625_FileID ((intptr_t)0x659)
#define ll_1626_FileID ((intptr_t)0x65a)
#define ll_1627_FileID ((intptr_t)0x65b)
#define ll_1628_FileID ((intptr_t)0x65c)
#define ll_1629_FileID ((intptr_t)0x65d)
#define ll_1630_FileID ((intptr_t)0x65e)
#define ll_1631_FileID ((intptr_t)0x65f)
#define ll_1632_FileID ((intptr_t)0x660)
#define ll_1633_FileID ((intptr_t)0x661)
#define ll_1634_FileID ((intptr_t)0x662)
#define ll_1635_FileID ((intptr_t)0x663)
#define ll_1636_FileID ((intptr_t)0x664)
#define ll_1637_FileID ((intptr_t)0x665)
#define ll_1638_FileID ((intptr_t)0x666)
#define ll_1639_FileID ((intptr_t)0x667)
#define ll_1640_FileID ((intptr_t)0x668)
#define ll_1641_FileID ((intptr_t)0x669)
#define ll_1642_FileID ((intptr_t)0x66a)
#define ll_1643_FileID ((intptr_t)0x66b)
#define ll_1644_FileID ((intptr_t)0x66c)
#define ll_1645_FileID ((intptr_t)0x66d)
#define ll_1646_FileID ((intptr_t)0x66e)
#define ll_1647_FileID ((intptr_t)0x66f)
#define ll_1648_FileID ((intptr_t)0x670)
#define ll_1649_FileID ((intptr_t)0x671)
#define ll_1650_FileID ((intptr_t)0x672)
#define ll_1651_FileID ((intptr_t)0x673)
#define ll_1652_FileID ((intptr_t)0x674)
#define ll_1653_FileID ((intptr_t)0x675)
#define ll_1654_FileID ((intptr_t)0x676)
#define ll_1655_FileID ((intptr_t)0x677)
#define ll_1656_FileID ((intptr_t)0x678)
#define ll_1657_FileID ((intptr_t)0x679)
#define ll_1658_FileID ((intptr_t)0x67a)
#define ll_1659_FileID ((intptr_t)0x67b)
#define ll_1660_FileID ((intptr_t)0x67c)
#define ll_1661_FileID ((intptr_t)0x67d)
#define ll_1662_FileID ((intptr_t)0x67e)
#define ll_1663_FileID ((intptr_t)0x67f)
#define ll_1664_FileID ((intptr_t)0x680)
#define ll_1665_FileID ((intptr_t)0x681)
#define ll_1666_FileID ((intptr_t)0x682)
#define ll_1667_FileID ((intptr_t)0x683)
#define ll_1668_FileID ((intptr_t)0x684)
#define ll_1669_FileID ((intptr_t)0x685)
#define ll_1670_FileID ((intptr_t)0x686)
#define ll_1671_FileID ((intptr_t)0x687)
#define ll_1672_FileID ((intptr_t)0x688)
#define ll_1673_FileID ((intptr_t)0x689)
#define ll_1674_FileID ((intptr_t)0x68a)
#define ll_1675_FileID ((intptr_t)0x68b)
#define ll_1676_FileID ((intptr_t)0x68c)
#define ll_1677_FileID ((intptr_t)0x68d)
#define ll_1678_FileID ((intptr_t)0x68e)
#define ll_1679_FileID ((intptr_t)0x68f)
#define ll_1680_FileID ((intptr_t)0x690)
#define ll_1681_FileID ((intptr_t)0x691)
#define ll_1682_FileID ((intptr_t)0x692)
#define ll_1683_FileID ((intptr_t)0x693)
#define ll_1684_FileID ((intptr_t)0x694)
#define ll_1685_FileID ((intptr_t)0x695)
#define ll_1686_FileID ((intptr_t)0x696)
#define ll_1687_FileID ((intptr_t)0x697)
#define ll_1688_FileID ((intptr_t)0x698)
#define ll_1689_FileID ((intptr_t)0x699)
#define ll_1690_FileID ((intptr_t)0x69a)
#define ll_1691_FileID ((intptr_t)0x69b)
#define ll_1692_FileID ((intptr_t)0x69c)
#define ll_1693_FileID ((intptr_t)0x69d)
#define ll_1694_FileID ((intptr_t)0x69e)
#define ll_1695_FileID ((intptr_t)0x69f)
#define ll_1696_FileID ((intptr_t)0x6a0)
#define ll_1697_FileID ((intptr_t)0x6a1)
#define ll_1698_FileID ((intptr_t)0x6a2)
#define ll_1699_FileID ((intptr_t)0x6a3)
#define ll_1700_FileID ((intptr_t)0x6a4)
#define ll_1701_FileID ((intptr_t)0x6a5)
#define ll_1702_FileID ((intptr_t)0x6a6)
#define ll_1703_FileID ((intptr_t)0x6a7)
#define ll_1704_FileID ((intptr_t)0x6a8)
#define ll_1705_FileID ((intptr_t)0x6a9)
#define ll_1706_FileID ((intptr_t)0x6aa)
#define ll_1707_FileID ((intptr_t)0x6ab)
#define ll_1708_FileID ((intptr_t)0x6ac)
#define ll_1709_FileID ((intptr_t)0x6ad)
#define ll_1710_FileID ((intptr_t)0x6ae)
#define ll_1711_FileID ((intptr_t)0x6af)
#define ll_1712_FileID ((intptr_t)0x6b0)
#define ll_1713_FileID ((intptr_t)0x6b1)
#define ll_1714_FileID ((intptr_t)0x6b2)
#define ll_1715_FileID ((intptr_t)0x6b3)
#define ll_1716_FileID ((intptr_t)0x6b4)
#define ll_1717_FileID ((intptr_t)0x6b5)
#define ll_1718_FileID ((intptr_t)0x6b6)
#define ll_1719_FileID ((intptr_t)0x6b7)
#define ll_1720_FileID ((intptr_t)0x6b8)
#define ll_1721_FileID ((intptr_t)0x6b9)
#define ll_1722_FileID ((intptr_t)0x6ba)
#define ll_1723_FileID ((intptr_t)0x6bb)
#define ll_1724_FileID ((intptr_t)0x6bc)
#define ll_1725_FileID ((intptr_t)0x6bd)
#define ll_1726_FileID ((intptr_t)0x6be)
#define ll_1727_FileID ((intptr_t)0x6bf)
#define ll_1728_FileID ((intptr_t)0x6c0)
#define ll_1729_FileID ((intptr_t)0x6c1)
#define ll_1730_FileID ((intptr_t)0x6c2)
#define ll_1731_FileID ((intptr_t)0x6c3)
#define ll_1732_FileID ((intptr_t)0x6c4)
#define ll_1733_FileID ((intptr_t)0x6c5)
#define ll_1734_FileID ((intptr_t)0x6c6)
#define ll_1735_FileID ((intptr_t)0x6c7)
#define ll_1736_FileID ((intptr_t)0x6c8)
#define ll_1737_FileID ((intptr_t)0x6c9)
#define ll_1738_FileID ((intptr_t)0x6ca)
#define ll_1739_FileID ((intptr_t)0x6cb)
#define ll_1740_FileID ((intptr_t)0x6cc)
#define ll_1741_FileID ((intptr_t)0x6cd)
#define ll_1742_FileID ((intptr_t)0x6ce)
#define ll_1743_FileID ((intptr_t)0x6cf)
#define ll_1744_FileID ((intptr_t)0x6d0)
#define ll_1745_FileID ((intptr_t)0x6d1)
#define ll_1746_FileID ((intptr_t)0x6d2)
#define ll_1747_FileID ((intptr_t)0x6d3)
#define ll_1748_FileID ((intptr_t)0x6d4)
#define ll_1749_FileID ((intptr_t)0x6d5)
#define ll_1750_FileID ((intptr_t)0x6d6)
#define ll_1751_FileID ((intptr_t)0x6d7)
#define ll_1752_FileID ((intptr_t)0x6d8)
#define ll_1753_FileID ((intptr_t)0x6d9)
#define ll_1754_FileID ((intptr_t)0x6da)
#define ll_1755_FileID ((intptr_t)0x6db)
#define ll_1756_FileID ((intptr_t)0x6dc)
#define ll_1757_FileID ((intptr_t)0x6dd)
#define ll_1758_FileID ((intptr_t)0x6de)
#define ll_1759_FileID ((intptr_t)0x6df)
#define ll_1760_FileID ((intptr_t)0x6e0)
#define ll_1761_FileID ((intptr_t)0x6e1)
#define ll_1762_FileID ((intptr_t)0x6e2)
#define ll_1763_FileID ((intptr_t)0x6e3)
#define ll_1764_FileID ((intptr_t)0x6e4)
#define ll_1765_FileID ((intptr_t)0x6e5)
#define ll_1766_FileID ((intptr_t)0x6e6)
#define ll_1767_FileID ((intptr_t)0x6e7)
#define ll_1768_FileID ((intptr_t)0x6e8)
#define ll_1769_FileID ((intptr_t)0x6e9)
#define ll_1770_FileID ((intptr_t)0x6ea)
#define ll_1771_FileID ((intptr_t)0x6eb)
#define ll_1772_FileID ((intptr_t)0x6ec)
#define ll_1773_FileID ((intptr_t)0x6ed)
#define ll_1774_FileID ((intptr_t)0x6ee)
#define ll_1775_FileID ((intptr_t)0x6ef)
#define ll_1776_FileID ((intptr_t)0x6f0)
#define ll_1777_FileID ((intptr_t)0x6f1)
#define ll_1778_FileID ((intptr_t)0x6f2)
#define ll_1779_FileID ((intptr_t)0x6f3)
#define ll_1780_FileID ((intptr_t)0x6f4)
#define ll_1781_FileID ((intptr_t)0x6f5)
#define ll_1782_FileID ((intptr_t)0x6f6)
#define ll_1783_FileID ((intptr_t)0x6f7)
#define ll_1784_FileID ((intptr_t)0x6f8)
#define ll_1785_FileID ((intptr_t)0x6f9)
#define ll_1786_FileID ((intptr_t)0x6fa)
#define ll_1787_FileID ((intptr_t)0x6fb)
#define ll_1788_FileID ((intptr_t)0x6fc)
#define ll_1789_FileID ((intptr_t)0x6fd)
#define ll_1790_FileID ((intptr_t)0x6fe)
#define ll_1791_FileID ((intptr_t)0x6ff)
#define ll_1792_FileID ((intptr_t)0x700)
#define ll_1793_FileID ((intptr_t)0x701)
#define ll_1794_FileID ((intptr_t)0x702)
#define ll_1795_FileID ((intptr_t)0x703)
#define ll_1796_FileID ((intptr_t)0x704)
#define ll_1797_FileID ((intptr_t)0x705)
#define ll_1798_FileID ((intptr_t)0x706)
#define ll_1799_FileID ((intptr_t)0x707)
#define ll_1800_FileID ((intptr_t)0x708)
#define ll_1801_FileID ((intptr_t)0x709)
#define ll_1802_FileID ((intptr_t)0x70a)
#define ll_1803_FileID ((intptr_t)0x70b)
#define ll_1804_FileID ((intptr_t)0x70c)
#define ll_1805_FileID ((intptr_t)0x70d)
#define ll_1806_FileID ((intptr_t)0x70e)
#define ll_1807_FileID ((intptr_t)0x70f)
#define ll_1808_FileID ((intptr_t)0x710)
#define ll_1809_FileID ((intptr_t)0x711)
#define ll_1810_FileID ((intptr_t)0x712)
#define ll_1811_FileID ((intptr_t)0x713)
#define ll_1812_FileID ((intptr_t)0x714)
#define ll_1813_FileID ((intptr_t)0x715)
#define ll_1814_FileID ((intptr_t)0x716)
#define ll_1815_FileID ((intptr_t)0x717)
#define ll_1816_FileID ((intptr_t)0x718)
#define ll_1817_FileID ((intptr_t)0x719)
#define ll_1818_FileID ((intptr_t)0x71a)
#define ll_1819_FileID ((intptr_t)0x71b)
#define ll_1820_FileID ((intptr_t)0x71c)
#define ll_1821_FileID ((intptr_t)0x71d)
#define ll_1822_FileID ((intptr_t)0x71e)
#define ll_1823_FileID ((intptr_t)0x71f)
#define ll_1824_FileID ((intptr_t)0x720)
#define ll_1825_FileID ((intptr_t)0x721)
#define ll_1826_FileID ((intptr_t)0x722)
#define ll_1827_FileID ((intptr_t)0x723)
#define ll_1828_FileID ((intptr_t)0x724)
#define ll_1829_FileID ((intptr_t)0x725)
#define ll_1830_FileID ((intptr_t)0x726)
#define ll_1831_FileID ((intptr_t)0x727)
#define ll_1832_FileID ((intptr_t)0x728)
#define ll_1833_FileID ((intptr_t)0x729)
#define ll_1834_FileID ((intptr_t)0x72a)
#define ll_1835_FileID ((intptr_t)0x72b)
#define ll_1836_FileID ((intptr_t)0x72c)
#define ll_1837_FileID ((intptr_t)0x72d)
#define ll_1838_FileID ((intptr_t)0x72e)
#define ll_1839_FileID ((intptr_t)0x72f)
#define ll_1840_FileID ((intptr_t)0x730)
#define ll_1841_FileID ((intptr_t)0x731)
#define ll_1842_FileID ((intptr_t)0x732)
#define ll_1843_FileID ((intptr_t)0x733)
#define ll_1844_FileID ((intptr_t)0x734)
#define ll_1845_FileID ((intptr_t)0x735)
#define ll_1846_FileID ((intptr_t)0x736)
#define ll_1847_FileID ((intptr_t)0x737)
#define ll_1848_FileID ((intptr_t)0x738)
#define ll_1849_FileID ((intptr_t)0x739)
#define ll_1850_FileID ((intptr_t)0x73a)
#define ll_1851_FileID ((intptr_t)0x73b)
#define ll_1852_FileID ((intptr_t)0x73c)
#define ll_1853_FileID ((intptr_t)0x73d)
#define ll_1854_FileID ((intptr_t)0x73e)
#define ll_1855_FileID ((intptr_t)0x73f)
#define ll_1856_FileID ((intptr_t)0x740)
#define ll_1857_FileID ((intptr_t)0x741)
#define ll_1858_FileID ((intptr_t)0x742)
#define ll_1859_FileID ((intptr_t)0x743)
#define ll_1860_FileID ((intptr_t)0x744)
#define ll_1861_FileID ((intptr_t)0x745)
#define ll_1862_FileID ((intptr_t)0x746)
#define ll_1863_FileID ((intptr_t)0x747)
#define ll_1864_FileID ((intptr_t)0x748)
#define ll_1865_FileID ((intptr_t)0x749)
#define ll_1866_FileID ((intptr_t)0x74a)
#define ll_1867_FileID ((intptr_t)0x74b)
#define ll_1868_FileID ((intptr_t)0x74c)
#define ll_1869_FileID ((intptr_t)0x74d)
#define ll_1870_FileID ((intptr_t)0x74e)
#define ll_1871_FileID ((intptr_t)0x74f)
#define ll_1872_FileID ((intptr_t)0x750)
#define ll_1873_FileID ((intptr_t)0x751)
#define ll_1874_FileID ((intptr_t)0x752)
#define ll_1875_FileID ((intptr_t)0x753)
#define ll_1876_FileID ((intptr_t)0x754)
#define ll_1877_FileID ((intptr_t)0x755)
#define ll_1878_FileID ((intptr_t)0x756)
#define ll_1879_FileID ((intptr_t)0x757)
#define ll_1880_FileID ((intptr_t)0x758)
#define ll_1881_FileID ((intptr_t)0x759)
#define ll_1882_FileID ((intptr_t)0x75a)
#define ll_1883_FileID ((intptr_t)0x75b)
#define ll_1884_FileID ((intptr_t)0x75c)
#define ll_1885_FileID ((intptr_t)0x75d)
#define ll_1886_FileID ((intptr_t)0x75e)
#define ll_1887_FileID ((intptr_t)0x75f)
#define ll_1888_FileID ((intptr_t)0x760)
#define ll_1889_FileID ((intptr_t)0x761)
#define ll_1890_FileID ((intptr_t)0x762)
#define ll_1891_FileID ((intptr_t)0x763)
#define ll_1892_FileID ((intptr_t)0x764)
#define ll_1893_FileID ((intptr_t)0x765)
#define ll_1894_FileID ((intptr_t)0x766)
#define ll_1895_FileID ((intptr_t)0x767)
#define ll_1896_FileID ((intptr_t)0x768)
#define ll_1897_FileID ((intptr_t)0x769)
#define ll_1898_FileID ((intptr_t)0x76a)
#define ll_1899_FileID ((intptr_t)0x76b)
#define ll_1900_FileID ((intptr_t)0x76c)
#define ll_1901_FileID ((intptr_t)0x76d)
#define ll_1902_FileID ((intptr_t)0x76e)
#define ll_1903_FileID ((intptr_t)0x76f)
#define ll_1904_FileID ((intptr_t)0x770)
#define ll_1905_FileID ((intptr_t)0x771)
#define ll_1906_FileID ((intptr_t)0x772)
#define ll_1907_FileID ((intptr_t)0x773)
#define ll_1908_FileID ((intptr_t)0x774)
#define ll_1909_FileID ((intptr_t)0x775)
#define ll_1910_FileID ((intptr_t)0x776)
#define ll_1911_FileID ((intptr_t)0x777)
#define ll_1912_FileID ((intptr_t)0x778)
#define ll_1913_FileID ((intptr_t)0x779)
#define ll_1914_FileID ((intptr_t)0x77a)
#define ll_1915_FileID ((intptr_t)0x77b)
#define ll_1916_FileID ((intptr_t)0x77c)
#define ll_1917_FileID ((intptr_t)0x77d)
#define ll_1918_FileID ((intptr_t)0x77e)
#define ll_1919_FileID ((intptr_t)0x77f)
#define ll_1920_FileID ((intptr_t)0x780)
#define ll_1921_FileID ((intptr_t)0x781)
#define ll_1922_FileID ((intptr_t)0x782)
#define ll_1923_FileID ((intptr_t)0x783)
#define ll_1924_FileID ((intptr_t)0x784)
#define ll_1925_FileID ((intptr_t)0x785)
#define ll_1926_FileID ((intptr_t)0x786)
#define ll_1927_FileID ((intptr_t)0x787)
#define ll_1928_FileID ((intptr_t)0x788)
#define ll_1929_FileID ((intptr_t)0x789)
#define ll_1930_FileID ((intptr_t)0x78a)
#define ll_1931_FileID ((intptr_t)0x78b)
#define ll_1932_FileID ((intptr_t)0x78c)
#define ll_1933_FileID ((intptr_t)0x78d)
#define ll_1934_FileID ((intptr_t)0x78e)
#define ll_1935_FileID ((intptr_t)0x78f)
#define ll_1936_FileID ((intptr_t)0x790)
#define ll_1937_FileID ((intptr_t)0x791)
#define ll_1938_FileID ((intptr_t)0x792)
#define ll_1939_FileID ((intptr_t)0x793)
#define ll_1940_FileID ((intptr_t)0x794)
#define ll_1941_FileID ((intptr_t)0x795)
#define ll_1942_FileID ((intptr_t)0x796)
#define ll_1943_FileID ((intptr_t)0x797)
#define ll_1944_FileID ((intptr_t)0x798)
#define ll_1945_FileID ((intptr_t)0x799)
#define ll_1946_FileID ((intptr_t)0x79a)
#define ll_1947_FileID ((intptr_t)0x79b)
#define ll_1948_FileID ((intptr_t)0x79c)
#define ll_1949_FileID ((intptr_t)0x79d)
#define ll_1950_FileID ((intptr_t)0x79e)
#define ll_1951_FileID ((intptr_t)0x79f)
#define ll_1952_FileID ((intptr_t)0x7a0)
#define ll_1953_FileID ((intptr_t)0x7a1)
#define ll_1954_FileID ((intptr_t)0x7a2)
#define ll_1955_FileID ((intptr_t)0x7a3)
#define ll_1956_FileID ((intptr_t)0x7a4)
#define ll_1957_FileID ((intptr_t)0x7a5)
#define ll_1958_FileID ((intptr_t)0x7a6)
#define ll_1959_FileID ((intptr_t)0x7a7)
#define ll_1960_FileID ((intptr_t)0x7a8)
#define ll_1961_FileID ((intptr_t)0x7a9)
#define ll_1962_FileID ((intptr_t)0x7aa)
#define ll_1963_FileID ((intptr_t)0x7ab)
#define ll_1964_FileID ((intptr_t)0x7ac)
#define ll_1965_FileID ((intptr_t)0x7ad)
#define ll_1966_FileID ((intptr_t)0x7ae)
#define ll_1967_FileID ((intptr_t)0x7af)
#define ll_1968_FileID ((intptr_t)0x7b0)
#define ll_1969_FileID ((intptr_t)0x7b1)
#define ll_1970_FileID ((intptr_t)0x7b2)
#define ll_1971_FileID ((intptr_t)0x7b3)
#define ll_1972_FileID ((intptr_t)0x7b4)
#define ll_1973_FileID ((intptr_t)0x7b5)
#define ll_1974_FileID ((intptr_t)0x7b6)
#define ll_1975_FileID ((intptr_t)0x7b7)
#define ll_1976_FileID ((intptr_t)0x7b8)
#define ll_1977_FileID ((intptr_t)0x7b9)
#define ll_1978_FileID ((intptr_t)0x7ba)
#define ll_1979_FileID ((intptr_t)0x7bb)
#define ll_1980_FileID ((intptr_t)0x7bc)
#define ll_1981_FileID ((intptr_t)0x7bd)
#define ll_1982_FileID ((intptr_t)0x7be)
#define ll_1983_FileID ((intptr_t)0x7bf)
#define ll_1984_FileID ((intptr_t)0x7c0)
#define ll_1985_FileID ((intptr_t)0x7c1)
#define ll_1986_FileID ((intptr_t)0x7c2)
#define ll_1987_FileID ((intptr_t)0x7c3)
#define ll_1988_FileID ((intptr_t)0x7c4)
#define ll_1989_FileID ((intptr_t)0x7c5)
#define ll_1990_FileID ((intptr_t)0x7c6)
#define ll_1991_FileID ((intptr_t)0x7c7)
#define ll_1992_FileID ((intptr_t)0x7c8)
#define ll_1993_FileID ((intptr_t)0x7c9)
#define ll_1994_FileID ((intptr_t)0x7ca)
#define ll_1995_FileID ((intptr_t)0x7cb)
#define ll_1996_FileID ((intptr_t)0x7cc)
#define ll_1997_FileID ((intptr_t)0x7cd)
#define ll_1998_FileID ((intptr_t)0x7ce)
#define ll_1999_FileID ((intptr_t)0x7cf)
#define ll_2000_FileID ((intptr_t)0x7d0)
#define ll_2001_FileID ((intptr_t)0x7d1)
#define ll_2002_FileID ((intptr_t)0x7d2)
#define ll_2003_FileID ((intptr_t)0x7d3)
#define ll_2004_FileID ((intptr_t)0x7d4)
#define ll_2005_FileID ((intptr_t)0x7d5)
#define ll_2006_FileID ((intptr_t)0x7d6)
#define ll_2007_FileID ((intptr_t)0x7d7)
#define ll_2008_FileID ((intptr_t)0x7d8)
#define ll_2009_FileID ((intptr_t)0x7d9)
#define ll_2010_FileID ((intptr_t)0x7da)
#define ll_2011_FileID ((intptr_t)0x7db)
#define ll_2012_FileID ((intptr_t)0x7dc)
#define ll_2013_FileID ((intptr_t)0x7dd)
#define ll_2014_FileID ((intptr_t)0x7de)
#define ll_2015_FileID ((intptr_t)0x7df)
#define ll_2016_FileID ((intptr_t)0x7e0)
#define ll_2017_FileID ((intptr_t)0x7e1)
#define ll_2018_FileID ((intptr_t)0x7e2)
#define ll_2019_FileID ((intptr_t)0x7e3)
#define ll_2020_FileID ((intptr_t)0x7e4)
#define ll_2021_FileID ((intptr_t)0x7e5)
#define ll_2022_FileID ((intptr_t)0x7e6)
#define ll_2023_FileID ((intptr_t)0x7e7)
#define ll_2024_FileID ((intptr_t)0x7e8)
#define ll_2025_FileID ((intptr_t)0x7e9)
#define ll_2026_FileID ((intptr_t)0x7ea)
#define ll_2027_FileID ((intptr_t)0x7eb)
#define ll_2028_FileID ((intptr_t)0x7ec)
#define ll_2029_FileID ((intptr_t)0x7ed)
#define ll_2030_FileID ((intptr_t)0x7ee)
#define ll_2031_FileID ((intptr_t)0x7ef)
#define ll_2032_FileID ((intptr_t)0x7f0)
#define ll_2033_FileID ((intptr_t)0x7f1)
#define ll_2034_FileID ((intptr_t)0x7f2)
#define ll_2035_FileID ((intptr_t)0x7f3)
#define ll_2036_FileID ((intptr_t)0x7f4)
#define ll_2037_FileID ((intptr_t)0x7f5)
#define ll_2038_FileID ((intptr_t)0x7f6)
#define ll_2039_FileID ((intptr_t)0x7f7)
#define ll_2040_FileID ((intptr_t)0x7f8)
#define ll_2041_FileID ((intptr_t)0x7f9)
#define ll_2042_FileID ((intptr_t)0x7fa)
#define ll_2043_FileID ((intptr_t)0x7fb)
#define ll_2044_FileID ((intptr_t)0x7fc)
#define ll_2045_FileID ((intptr_t)0x7fd)
#define ll_2046_FileID ((intptr_t)0x7fe)
#define ll_2047_FileID ((intptr_t)0x7ff)
#define ll_2048_FileID ((intptr_t)0x800)
#define ll_2049_FileID ((intptr_t)0x801)
#define ll_2050_FileID ((intptr_t)0x802)
#define ll_2051_FileID ((intptr_t)0x803)
#define ll_2052_FileID ((intptr_t)0x804)
#define ll_2053_FileID ((intptr_t)0x805)
#define ll_2054_FileID ((intptr_t)0x806)
#define ll_2055_FileID ((intptr_t)0x807)
#define ll_2056_FileID ((intptr_t)0x808)
#define ll_2057_FileID ((intptr_t)0x809)
#define ll_2058_FileID ((intptr_t)0x80a)
#define ll_2059_FileID ((intptr_t)0x80b)
#define ll_2060_FileID ((intptr_t)0x80c)
#define ll_2061_FileID ((intptr_t)0x80d)
#define ll_2062_FileID ((intptr_t)0x80e)
#define ll_2063_FileID ((intptr_t)0x80f)
#define ll_2064_FileID ((intptr_t)0x810)
#define ll_2065_FileID ((intptr_t)0x811)
#define ll_2066_FileID ((intptr_t)0x812)
#define ll_2067_FileID ((intptr_t)0x813)
#define ll_2068_FileID ((intptr_t)0x814)
#define ll_2069_FileID ((intptr_t)0x815)
#define ll_2070_FileID ((intptr_t)0x816)
#define ll_2071_FileID ((intptr_t)0x817)
#define ll_2072_FileID ((intptr_t)0x818)
#define ll_2073_FileID ((intptr_t)0x819)
#define ll_2074_FileID ((intptr_t)0x81a)
#define ll_2075_FileID ((intptr_t)0x81b)
#define ll_2076_FileID ((intptr_t)0x81c)
#define ll_2077_FileID ((intptr_t)0x81d)
#define ll_2078_FileID ((intptr_t)0x81e)
#define ll_2079_FileID ((intptr_t)0x81f)
#define ll_2080_FileID ((intptr_t)0x820)
#define ll_2081_FileID ((intptr_t)0x821)
#define ll_2082_FileID ((intptr_t)0x822)
#define ll_2083_FileID ((intptr_t)0x823)
#define ll_2084_FileID ((intptr_t)0x824)
#define ll_2085_FileID ((intptr_t)0x825)
#define ll_2086_FileID ((intptr_t)0x826)
#define ll_2087_FileID ((intptr_t)0x827)
#define ll_2088_FileID ((intptr_t)0x828)
#define ll_2089_FileID ((intptr_t)0x829)
#define ll_2090_FileID ((intptr_t)0x82a)
#define ll_2091_FileID ((intptr_t)0x82b)
#define ll_2092_FileID ((intptr_t)0x82c)
#define ll_2093_FileID ((intptr_t)0x82d)
#define ll_2094_FileID ((intptr_t)0x82e)
#define ll_2095_FileID ((intptr_t)0x82f)
#define ll_2096_FileID ((intptr_t)0x830)
#define ll_2097_FileID ((intptr_t)0x831)
#define ll_2098_FileID ((intptr_t)0x832)
#define ll_2099_FileID ((intptr_t)0x833)
#define ll_2100_FileID ((intptr_t)0x834)
#define ll_2101_FileID ((intptr_t)0x835)
#define ll_2102_FileID ((intptr_t)0x836)
#define ll_2103_FileID ((intptr_t)0x837)
#define ll_2104_FileID ((intptr_t)0x838)
#define ll_2105_FileID ((intptr_t)0x839)
#define ll_2106_FileID ((intptr_t)0x83a)
#define ll_2107_FileID ((intptr_t)0x83b)
#define ll_2108_FileID ((intptr_t)0x83c)
#define ll_2109_FileID ((intptr_t)0x83d)
#define ll_2110_FileID ((intptr_t)0x83e)
#define ll_2111_FileID ((intptr_t)0x83f)
#define ll_2112_FileID ((intptr_t)0x840)
#define ll_2113_FileID ((intptr_t)0x841)
#define ll_2114_FileID ((intptr_t)0x842)
#define ll_2115_FileID ((intptr_t)0x843)
#define ll_2116_FileID ((intptr_t)0x844)
#define ll_2117_FileID ((intptr_t)0x845)
#define ll_2118_FileID ((intptr_t)0x846)
#define ll_2119_FileID ((intptr_t)0x847)
#define ll_2120_FileID ((intptr_t)0x848)
#define ll_2121_FileID ((intptr_t)0x849)
#define ll_2122_FileID ((intptr_t)0x84a)
#define ll_2123_FileID ((intptr_t)0x84b)
#define ll_2124_FileID ((intptr_t)0x84c)
#define ll_2125_FileID ((intptr_t)0x84d)
#define ll_2126_FileID ((intptr_t)0x84e)
#define ll_2127_FileID ((intptr_t)0x84f)
#define ll_2128_FileID ((intptr_t)0x850)
#define ll_2129_FileID ((intptr_t)0x851)
#define ll_2130_FileID ((intptr_t)0x852)
#define ll_2131_FileID ((intptr_t)0x853)
#define llMNCommonOptionTabLeftSprite ((intptr_t)0x1e8)
#define llMNCommonOptionTabMiddleSprite ((intptr_t)0x330)
#define llMNCommonOptionTabRightSprite ((intptr_t)0x568)
#define llMNCommonFrameSprite ((intptr_t)0x1420)
#define llMNCommonOnTextJapSprite ((intptr_t)0x15c0)
#define llMNCommonOffTextJapSprite ((intptr_t)0x1760)
#define llMNCommonAutoTextJapSprite ((intptr_t)0x1968)
#define llMNCommonDecalPaperSprite ((intptr_t)0x2a30)
#define llMNCommonSmashLogoSprite ((intptr_t)0x31f8)
#define llMNCommonFightBackgroundSprite ((intptr_t)0xb6d0)
#define llMNCommonOnTextSprite ((intptr_t)0xb818)
#define llMNCommonOffTextSprite ((intptr_t)0xb958)
#define llMNCommonSlashSprite ((intptr_t)0xba28)
#define llMNCommonGameModeTextSprite ((intptr_t)0xd240)
#define llMNCommonDigit0Sprite ((intptr_t)0xd310)
#define llMNCommonDigit1Sprite ((intptr_t)0xd3e0)
#define llMNCommonDigit2Sprite ((intptr_t)0xd4b0)
#define llMNCommonDigit3Sprite ((intptr_t)0xd580)
#define llMNCommonDigit4Sprite ((intptr_t)0xd650)
#define llMNCommonDigit5Sprite ((intptr_t)0xd720)
#define llMNCommonDigit6Sprite ((intptr_t)0xd7f0)
#define llMNCommonDigit7Sprite ((intptr_t)0xd8c0)
#define llMNCommonDigit8Sprite ((intptr_t)0xd990)
#define llMNCommonDigit9Sprite ((intptr_t)0xda60)
#define llMNCommonPercentageSprite ((intptr_t)0xdb30)
#define llMNCommonInfinitySprite ((intptr_t)0xdc48)
#define llMNCommonColonSprite ((intptr_t)0xdcf0)
#define llMNCommonArrowRSprite ((intptr_t)0xdd90)
#define llMNCommonArrowLSprite ((intptr_t)0xde30)
#define llMNCommonAutoTextSprite ((intptr_t)0xdf48)
#define llMNCommonSmashBrosCollageSprite ((intptr_t)0x18000)
#define llMNMain1PModeTextJapSprite ((intptr_t)0x4c8)
#define llMNMainVsModeTextJapSprite ((intptr_t)0x960)
#define llMNMainDataTextJapSprite ((intptr_t)0xe00)
#define llMNMainControllerIconSprite ((intptr_t)0x1990)
#define llMNMainConsoleIconSprite ((intptr_t)0x2520)
#define llMNMainDataIconSprite ((intptr_t)0x30b0)
#define llMNMainModeSelectTextSprite ((intptr_t)0x40f0)
#define llMNMainSettingsIconSprite ((intptr_t)0x4c80)
#define llMNMainOptionTextJapSprite ((intptr_t)0x5300)
#define llMNMain1PModeTextSprite ((intptr_t)0x5570)
#define llMNMainVsModeTextSprite ((intptr_t)0x57e0)
#define llMNMainDataTextSprite ((intptr_t)0x5980)
#define llMNMainControllerIconDarkSprite ((intptr_t)0x6048)
#define llMNMainConsoleIconDarkSprite ((intptr_t)0x6708)
#define llMNMainDataIconDarkSprite ((intptr_t)0x6dc8)
#define llMNMainDecalBarEdgeSprite ((intptr_t)0x72e8)
#define llMNMainSmashLogoSprite ((intptr_t)0x7aa8)
#define llMNMainDecalBarMiddleSprite ((intptr_t)0x7c38)
#define llMNMainSettingsIconDarkSprite ((intptr_t)0x82f8)
#define llMNMainOptionTextSprite ((intptr_t)0x84f8)
#define llMN1P1PGameTextJapSprite ((intptr_t)0x4c8)
#define llMN1POptionTabSprite ((intptr_t)0x1108)
#define llMN1PTrainingModeTextJapSprite ((intptr_t)0x1618)
#define llMN1PBonus1PracticeTextJapSprite ((intptr_t)0x1df8)
#define llMN1PBonus2PracticeTextJapSprite ((intptr_t)0x25d8)
#define llMN1P1PGameTextSprite ((intptr_t)0x2a28)
#define llMN1PControllerIconDarkSprite ((intptr_t)0x50f8)
#define llMN1P1PTextSprite ((intptr_t)0x5338)
#define llMN1PTrainingModeTextSprite ((intptr_t)0x5ac8)
#define llMN1PBonus1PracticeTextSprite ((intptr_t)0x5f28)
#define llMN1PBonus2PracticeTextSprite ((intptr_t)0x6388)
#define llMNIndexBubbleLeftSprite ((intptr_t)0xf0)
#define llMNIndexBubbleMiddleSprite ((intptr_t)0x230)
#define llMNIndexBubbleRightSprite ((intptr_t)0x370)
#define llMNIndexCharactersTextJapSprite ((intptr_t)0xa68)
#define llMNIndexVsRecordTextJapSprite ((intptr_t)0xe88)
#define llMNIndexSoundTextJapSprite ((intptr_t)0x12a8)
#define llMNIndexSoundTestTextJapSprite ((intptr_t)0x18a8)
#define llMNIndexScreenAdjustTextJapSprite ((intptr_t)0x1cc8)
#define llMNIndexIndexTextSprite ((intptr_t)0x27a0)
#define llMNIndexCharactersTextSprite ((intptr_t)0x2bc0)
#define llMNIndexVsRecordTextSprite ((intptr_t)0x2fe0)
#define llMNIndexSoundTextSprite ((intptr_t)0x32f0)
#define llMNIndexTestTextSprite ((intptr_t)0x34f0)
#define llMNIndexScreenAdjustTextSprite ((intptr_t)0x3b88)
#define llMNIndexDataIconDarkSprite ((intptr_t)0x6258)
#define llMNOptionSoundTextJapSprite ((intptr_t)0x3d8)
#define llMNOptionStereoTextJapSprite ((intptr_t)0x6a8)
#define llMNOptionMonoTextJapSprite ((intptr_t)0x978)
#define llMNOptionFlashingTextJapSprite ((intptr_t)0xff0)
#define llMNOptionScreenAdjustTextJapSprite ((intptr_t)0x1580)
#define llMNOptionBackupClearTextJapSprite ((intptr_t)0x1cf0)
#define llMNOptionWarningTextSprite ((intptr_t)0x21b8)
#define llMNOptionPhotosensitivityWarningTextJapSprite ((intptr_t)0x68b0)
#define llMNOptionWarningIconSprite ((intptr_t)0x6fd8)
#define llMNOptionStereoTextSprite ((intptr_t)0x71f8)
#define llMNOptionMonoTextSprite ((intptr_t)0x73a8)
#define llMNOptionSoundTextSprite ((intptr_t)0x7628)
#define llMNOptionFlashingTextSprite ((intptr_t)0x7aa8)
#define llMNOptionScreenAdjustTextSprite ((intptr_t)0x8138)
#define llMNOptionBackupClearTextSprite ((intptr_t)0x8780)
#define llMNOptionOptionTextSprite ((intptr_t)0x9288)
#define llMNOptionSettingsIconDarkSprite ((intptr_t)0xb958)
#define llMNDataCharactersTextJapSprite ((intptr_t)0x6a8)
#define llMNDataVSRecordTextJapSprite ((intptr_t)0xac8)
#define llMNDataSoundTestTextJapSprite ((intptr_t)0x10c8)
#define llMNDataCharactersTextSprite ((intptr_t)0x14e0)
#define llMNDataVSRecordTextSprite ((intptr_t)0x1900)
#define llMNDataSoundTestTextSprite ((intptr_t)0x1d20)
#define llMNDataDataTextSprite ((intptr_t)0x23a8)
#define llMNDataDataIconDarkSprite ((intptr_t)0x4a78)
#define llMNVSModeVSStartTextJapSprite ((intptr_t)0x540)
#define llMNVSModeBattleTextJapSprite ((intptr_t)0x800)
#define llMNVSModeTimeBasedTextJapSprite ((intptr_t)0xb38)
#define llMNVSModeStockTextJapSprite ((intptr_t)0xee0)
#define llMNVSModeTeamTextJapSprite ((intptr_t)0x11a0)
#define llMNVSModeTimeLimitTextJapSprite ((intptr_t)0x1550)
#define llMNVSModeRemaningPlayersTextJapSprite ((intptr_t)0x19f0)
#define llMNVSModeMatchOptionsTextJapSprite ((intptr_t)0x2070)
#define llMNVSModeVSStartTextSprite ((intptr_t)0x24c8)
#define llMNVSModeRulePeriodTextSprite ((intptr_t)0x2748)
#define llMNVSModeTimeTextSprite ((intptr_t)0x28e0)
#define llMNVSModeStockTextSprite ((intptr_t)0x2a80)
#define llMNVSModeTeamTextSprite ((intptr_t)0x2c20)
#define llMNVSModeTimePeriodTextSprite ((intptr_t)0x2ec8)
#define llMNVSModeMinTextSprite ((intptr_t)0x2fc8)
#define llMNVSModeStockPeriodTextSprite ((intptr_t)0x3248)
#define llMNVSModeVSOptionsTextSprite ((intptr_t)0x3828)
#define llMNVSModeConsoleIconDarkSprite ((intptr_t)0x5eb0)
#define llMNVSModeVSTextSprite ((intptr_t)0x6118)
#define llMNVSOptionsHandicapTextJapSprite ((intptr_t)0x360)
#define llMNVSOptionsTeamAttackTextJapSprite ((intptr_t)0x9e0)
#define llMNVSOptionsStageSelectTextJapSprite ((intptr_t)0xf70)
#define llMNVSOptionsDamageTextJapSprite ((intptr_t)0x15f0)
#define llMNVSOptionsItemSwitchTextJapSprite ((intptr_t)0x1c70)
#define llMNVSOptionsVSOptionsTextSprite ((intptr_t)0x2668)
#define llMNVSOptionsBubbleSprite ((intptr_t)0x33d8)
#define llMNVSOptionsHandicapTextSprite ((intptr_t)0x3690)
#define llMNVSOptionsTeamAttackTextSprite ((intptr_t)0x3968)
#define llMNVSOptionsStageSelectTextSprite ((intptr_t)0x3cf8)
#define llMNVSOptionsItemSwitchTextSprite ((intptr_t)0x3fc8)
#define llMNVSOptionsDamageTextSprite ((intptr_t)0x4228)
#define llMNVSOptionsConsoleIconDarkSprite ((intptr_t)0x5f60)
#define llMNVSItemSwitchLabelVSOptionsSprite ((intptr_t)0x9a8)
#define llMNVSItemSwitchLabelItemSwitchSprite ((intptr_t)0xb20)
#define llMNVSItemSwitchAppearanceNoneSprite ((intptr_t)0xce8)
#define llMNVSItemSwitchAppearanceVeryLowSprite ((intptr_t)0xea8)
#define llMNVSItemSwitchAppearanceLowSprite ((intptr_t)0xf98)
#define llMNVSItemSwitchAppearanceMiddleSprite ((intptr_t)0x10d0)
#define llMNVSItemSwitchAppearanceHighSprite ((intptr_t)0x11e8)
#define llMNVSItemSwitchAppearanceVeryHighSprite ((intptr_t)0x13a8)
#define llMNVSItemSwitchToggleOnSprite ((intptr_t)0x1488)
#define llMNVSItemSwitchToggleOffSprite ((intptr_t)0x1568)
#define llMNVSItemSwitchToggleSlashSprite ((intptr_t)0x1608)
#define llMNVSItemSwitchDecalButtonSprite ((intptr_t)0x3430)
#define llMNVSItemSwitchFrameSprite ((intptr_t)0x3948)
#define llMNVSItemSwitchItemListSprite ((intptr_t)0x5e60)
#define llMNVSItemSwitchCursorSprite ((intptr_t)0x63a8)
#define llMNVSItemSwitchAppearancePercentTextJapSprite ((intptr_t)0x65c0)
#define llMNVSItemSwitchBeamSwordTextJapSprite ((intptr_t)0x67e0)
#define llMNVSItemSwitchHomerunBatTextJapSprite ((intptr_t)0x6a00)
#define llMNVSItemSwitchHammerTextJapSprite ((intptr_t)0x6c20)
#define llMNVSItemSwitch0x6e40Sprite ((intptr_t)0x6e40)
#define llMNVSItemSwitchHarisenTextJapSprite ((intptr_t)0x7060)
#define llMNVSItemSwitch0x7280Sprite ((intptr_t)0x7280)
#define llMNVSItemSwitchBombTrooperTextJapSprite ((intptr_t)0x74a0)
#define llMNVSItemSwitchBumperTextJapSprite ((intptr_t)0x76c0)
#define llMNVSItemSwitchShellTextJapSprite ((intptr_t)0x78e0)
#define llMNVSItemSwitchMonsterBallTextJapSprite ((intptr_t)0x7b00)
#define llMNVSItemSwitchLayGunTextJapSprite ((intptr_t)0x7d20)
#define llMNVSItemSwitchFireFlowerTextJapSprite ((intptr_t)0x7f40)
#define llMNVSItemSwitchStarRodTextJapSprite ((intptr_t)0x8160)
#define llMNVSItemSwitchMaximTomatoTextJapSprite ((intptr_t)0x8380)
#define llMNVSItemSwitchHeartTextJapSprite ((intptr_t)0x85a0)
#define llMNVSItemSwitchStarTextJapSprite ((intptr_t)0x87c0)
#define llMNVSItemSwitchMotionSensorBombTextJapSprite ((intptr_t)0x8b98)
#define llMNVSItemSwitchVeryLowTextJapSprite ((intptr_t)0x8db0)
#define llMNVSItemSwitchLowTextJapSprite ((intptr_t)0x8fd0)
#define llMNVSItemSwitchMiddleTextJapSprite ((intptr_t)0x91f0)
#define llMNVSItemSwitchHighTextJapSprite ((intptr_t)0x9410)
#define llMNVSItemSwitchVeryHighTextJapSprite ((intptr_t)0x9630)
#define llMNVSItemSwitchNothingTextJapSprite ((intptr_t)0x9850)
#define llMNMessageUnlockLuigiSprite ((intptr_t)0x9e0)
#define llMNMessageUnlockNessSprite ((intptr_t)0x1148)
#define llMNMessageUnlockCaptainSprite ((intptr_t)0x1f50)
#define llMNMessageUnlockPurinSprite ((intptr_t)0x2e58)
#define llMNMessageUnlockInishieSprite ((intptr_t)0x3458)
#define llMNMessageUnlockSoundTestSprite ((intptr_t)0x4180)
#define llMNMessageUnlockItemSwitchSprite ((intptr_t)0x4eb0)
#define llMNMessageDecalExclaimSprite ((intptr_t)0x5300)
#define llSC1PChallengerChallengerTextSprite ((intptr_t)0x1f8)
#define llSC1PChallengerApproachingTextSprite ((intptr_t)0x488)
#define llSC1PChallengerWarningTextSprite ((intptr_t)0x968)
#define llSC1PChallengerDecalExclaimSprite ((intptr_t)0xdb0)
#define llSC1PIntroVSDecalSprite ((intptr_t)0x1f10)
#define llSC1PIntroNumber1Sprite ((intptr_t)0x2018)
#define llSC1PIntroNumber2Sprite ((intptr_t)0x2118)
#define llSC1PIntroNumber3Sprite ((intptr_t)0x2218)
#define llSC1PIntroNumber4Sprite ((intptr_t)0x2318)
#define llSC1PIntroNumber5Sprite ((intptr_t)0x2418)
#define llSC1PIntroNumber6Sprite ((intptr_t)0x2518)
#define llSC1PIntroNumber7Sprite ((intptr_t)0x2618)
#define llSC1PIntroNumber8Sprite ((intptr_t)0x2718)
#define llSC1PIntroNumber9Sprite ((intptr_t)0x2818)
#define llSC1PIntroNumber10Sprite ((intptr_t)0x29b8)
#define llSC1PIntroCrossSprite ((intptr_t)0x2b58)
#define llSC1PIntroStageTextSprite ((intptr_t)0x2e38)
#define llSC1PIntroBonusTextSprite ((intptr_t)0x30f8)
#define llSC1PIntroFinalTextSprite ((intptr_t)0x3320)
#define llSC1PIntroBreakTheTargetsTextSprite ((intptr_t)0x3b08)
#define llSC1PIntroBoardThePlatformsTextSprite ((intptr_t)0x4388)
#define llSC1PIntroRaceToTheFinishTextSprite ((intptr_t)0x4ac8)
#define llSC1PIntro0x5028Sprite ((intptr_t)0x5028)
#define llSC1PIntroDashSprite ((intptr_t)0x50e8)
#define llSC1PIntroMetalMarioTextSprite ((intptr_t)0x5328)
#define llSC1PIntroMasterHandTextSprite ((intptr_t)0x5568)
#define llSC1PIntroGiantDKTextSprite ((intptr_t)0x5748)
#define llSC1PIntroFoxMcCloudTextSprite ((intptr_t)0x5988)
#define llSC1PIntroKirbyTeamVS8TextSprite ((intptr_t)0x5c88)
#define llSC1PIntroMarioBrosTextSprite ((intptr_t)0x5ec8)
#define llSC1PIntroFightingPolygonTeamVS30TextSprite ((intptr_t)0x63f8)
#define llSC1PIntroSamusAranTextSprite ((intptr_t)0x6638)
#define llSC1PIntroYoshiTeamVS18TextSprite ((intptr_t)0x6938)
#define llSC1PIntroVSTextSprite ((intptr_t)0x69f8)
#define llSC1PIntroAllyTextSprite ((intptr_t)0x6b18)
#define llSC1PIntroAllyText2Sprite ((intptr_t)0x6c38)
#define llSC1PIntroFighterMarioCamAnimJoint ((intptr_t)0x6c80)
#define llSC1PIntroFighterFoxCamAnimJoint ((intptr_t)0x6cb0)
#define llSC1PIntroFighterDonkeyCamAnimJoint ((intptr_t)0x6ce0)
#define llSC1PIntroFighterSamusCamAnimJoint ((intptr_t)0x6d10)
#define llSC1PIntroFighterLuigiCamAnimJoint ((intptr_t)0x6d40)
#define llSC1PIntroFighterLinkCamAnimJoint ((intptr_t)0x6d70)
#define llSC1PIntroFighterYoshiCamAnimJoint ((intptr_t)0x6da0)
#define llSC1PIntroFighterCaptainCamAnimJoint ((intptr_t)0x6dd0)
#define llSC1PIntroFighterKirbyCamAnimJoint ((intptr_t)0x6e00)
#define llSC1PIntroFighterPikachuCamAnimJoint ((intptr_t)0x6e30)
#define llSC1PIntroFighterPurinCamAnimJoint ((intptr_t)0x6e60)
#define llSC1PIntroFighterNessCamAnimJoint ((intptr_t)0x6e90)
#define llSC1PIntroStageKirbyCamAnimJoint ((intptr_t)0x6ec0)
#define llSC1PIntroStageSamusCamAnimJoint ((intptr_t)0x6f50)
#define llSC1PIntroStageFoxCamAnimJoint ((intptr_t)0x6f80)
#define llSC1PIntroStagePikachuCamAnimJoint ((intptr_t)0x6fb0)
#define llSC1PIntroStageLinkCamAnimJoint ((intptr_t)0x6fe0)
#define llSC1PIntroStageYoshiCamAnimJoint ((intptr_t)0x6ef0)
#define llSC1PIntroStageBossCamAnimJoint ((intptr_t)0x6f20)
#define llSC1PIntroStageDonkeyCamAnimJoint ((intptr_t)0x7010)
#define llSC1PIntroStageMarioCamAnimJoint ((intptr_t)0x7040)
#define llSC1PIntroStageMMarioCamAnimJoint ((intptr_t)0x7070)
#define llSC1PIntroStageZakoCamAnimJoint ((intptr_t)0x70a0)
#define llSC1PIntroLinkMarkerSprite ((intptr_t)0x71d0)
#define llSC1PIntroYoshiMarkerSprite ((intptr_t)0x7320)
#define llSC1PIntroFoxMarkerSprite ((intptr_t)0x7470)
#define llSC1PIntroMarioBrosMarkerSprite ((intptr_t)0x75c0)
#define llSC1PIntroPikachuMarkerSprite ((intptr_t)0x7710)
#define llSC1PIntroDKMarkerSprite ((intptr_t)0x7860)
#define llSC1PIntroKirbyMarkerSprite ((intptr_t)0x79b0)
#define llSC1PIntroSamusMarkerSprite ((intptr_t)0x7b00)
#define llSC1PIntroMarioMarkerSprite ((intptr_t)0x7c50)
#define llSC1PIntroExclamationMarkSprite ((intptr_t)0x7d60)
#define llSC1PIntroBossMarkerSprite ((intptr_t)0x7e70)
#define llSC1PIntroBonusMarkerSprite ((intptr_t)0x7f40)
#define llSC1PIntroBannerTopSprite ((intptr_t)0xc898)
#define llSC1PIntroBannerBottomSprite ((intptr_t)0xed00)
#define llSC1PIntroSkySprite ((intptr_t)0x14bf0)
#define llCharacterNamesMarioSprite ((intptr_t)0x138)
#define llCharacterNamesFoxSprite ((intptr_t)0x258)
#define llCharacterNamesDonkeySprite ((intptr_t)0x378)
#define llCharacterNamesSamusSprite ((intptr_t)0x4f8)
#define llCharacterNamesLuigiSprite ((intptr_t)0x618)
#define llCharacterNamesLinkSprite ((intptr_t)0x738)
#define llCharacterNamesYoshiSprite ((intptr_t)0x858)
#define llCharacterNamesCaptainSprite ((intptr_t)0xa38)
#define llCharacterNamesKirbySprite ((intptr_t)0xbb8)
#define llCharacterNamesPikachuSprite ((intptr_t)0xd38)
#define llCharacterNamesPurinSprite ((intptr_t)0xf78)
#define llCharacterNamesNessSprite ((intptr_t)0x1098)
#define llBonusPictureTargetSprite ((intptr_t)0xe980)
#define llBonusPictureRaceSprite ((intptr_t)0x1a658)
#define llBonusPicturePlatformSprite ((intptr_t)0x27388)
#define llMNScreenAdjustInstructionSprite ((intptr_t)0x918)
#define llMNScreenAdjustGuideSprite ((intptr_t)0x98a0)
#define llMNCharactersLabelSprite ((intptr_t)0x630)
#define llMNCharactersNameTagDefaultSprite ((intptr_t)0x1230)
#define llMNCharactersNameTagTallSprite ((intptr_t)0x28f0)
#define llMNCharactersMarioNameSprite ((intptr_t)0x2f98)
#define llMNCharactersFoxNameSprite ((intptr_t)0x33a0)
#define llMNCharactersDonkeyNameSprite ((intptr_t)0x4290)
#define llMNCharactersSamusNameSprite ((intptr_t)0x4910)
#define llMNCharactersLuigiNameSprite ((intptr_t)0x4f78)
#define llMNCharactersLinkNameSprite ((intptr_t)0x5398)
#define llMNCharactersYoshiNameSprite ((intptr_t)0x58f8)
#define llMNCharactersCaptainNameSprite ((intptr_t)0x6828)
#define llMNCharactersKirbyNameSprite ((intptr_t)0x6e48)
#define llMNCharactersPikachuNameSprite ((intptr_t)0x7628)
#define llMNCharactersPurinNameSprite ((intptr_t)0x82e0)
#define llMNCharactersNessNameSprite ((intptr_t)0x8828)
#define llMNCharactersMarioStorySprite ((intptr_t)0xaca8)
#define llMNCharactersFoxStorySprite ((intptr_t)0xd128)
#define llMNCharactersDonkeyStorySprite ((intptr_t)0xf5a8)
#define llMNCharactersSamusStorySprite ((intptr_t)0x11a28)
#define llMNCharactersLuigiStorySprite ((intptr_t)0x13ea8)
#define llMNCharactersLinkStorySprite ((intptr_t)0x16328)
#define llMNCharactersYoshiStorySprite ((intptr_t)0x187a8)
#define llMNCharactersCaptainStorySprite ((intptr_t)0x1ac28)
#define llMNCharactersKirbyStorySprite ((intptr_t)0x1d0a8)
#define llMNCharactersPikachuStorySprite ((intptr_t)0x1f528)
#define llMNCharactersPurinStorySprite ((intptr_t)0x219a8)
#define llMNCharactersNessStorySprite ((intptr_t)0x23e28)
#define llMNCharactersWorksWallpaperSprite ((intptr_t)0x25058)
#define llMNCharactersMarioWorksSprite ((intptr_t)0x25ab8)
#define llMNCharactersFoxWorksSprite ((intptr_t)0x26518)
#define llMNCharactersDonkeyWorksSprite ((intptr_t)0x26f78)
#define llMNCharactersSamusWorksSprite ((intptr_t)0x279d8)
#define llMNCharactersLuigiWorksSprite ((intptr_t)0x28438)
#define llMNCharactersLinkWorksSprite ((intptr_t)0x28e98)
#define llMNCharactersYoshiWorksSprite ((intptr_t)0x298f8)
#define llMNCharactersCaptainWorksSprite ((intptr_t)0x2a358)
#define llMNCharactersKirbyWorksSprite ((intptr_t)0x2adb8)
#define llMNCharactersPikachuWorksSprite ((intptr_t)0x2b818)
#define llMNCharactersPurinWorksSprite ((intptr_t)0x2c278)
#define llMNCharactersNessWorksSprite ((intptr_t)0x2ccd8)
#define llMNCharactersMotionSpecialHiInputSprite ((intptr_t)0x2cda8)
#define llMNCharactersMotionSpecialNInputSprite ((intptr_t)0x2ce78)
#define llMNCharactersMotionSpecialLwInputSprite ((intptr_t)0x2cf48)
#define llMNCharactersMarioSpecialHiNameSprite ((intptr_t)0x2d088)
#define llMNCharactersFoxSpecialHiNameSprite ((intptr_t)0x2d1c8)
#define llMNCharactersDonkeySpecialHiNameSprite ((intptr_t)0x2d308)
#define llMNCharactersSamusSpecialHiNameSprite ((intptr_t)0x2d448)
#define llMNCharactersLinkSpecialHiNameSprite ((intptr_t)0x2d588)
#define llMNCharactersYoshiSpecialHiNameSprite ((intptr_t)0x2d6c8)
#define llMNCharactersCaptainSpecialHiNameSprite ((intptr_t)0x2d808)
#define llMNCharactersKirbySpecialHiNameSprite ((intptr_t)0x2d948)
#define llMNCharactersPikachuSpecialHiNameSprite ((intptr_t)0x2da88)
#define llMNCharactersPurinSpecialHiNameSprite ((intptr_t)0x2dbc8)
#define llMNCharactersNessSpecialHiNameSprite ((intptr_t)0x2dd08)
#define llMNCharactersMarioSpecialNNameSprite ((intptr_t)0x2de48)
#define llMNCharactersFoxSpecialNNameSprite ((intptr_t)0x2df88)
#define llMNCharactersDonkeySpecialNNameSprite ((intptr_t)0x2e0c8)
#define llMNCharactersSamusSpecialNNameSprite ((intptr_t)0x2e208)
#define llMNCharactersLinkSpecialNNameSprite ((intptr_t)0x2e348)
#define llMNCharactersYoshiSpecialNNameSprite ((intptr_t)0x2e488)
#define llMNCharactersCaptainSpecialNNameSprite ((intptr_t)0x2e5c8)
#define llMNCharactersKirbySpecialNNameSprite ((intptr_t)0x2e740)
#define llMNCharactersPikachuSpecialNNameSprite ((intptr_t)0x2e888)
#define llMNCharactersPurinSpecialNNameSprite ((intptr_t)0x2e9c8)
#define llMNCharactersNessSpecialNNameSprite ((intptr_t)0x2eb08)
#define llMNCharactersMarioSpecialLwNameSprite ((intptr_t)0x2ec48)
#define llMNCharactersFoxSpecialLwNameSprite ((intptr_t)0x2ed88)
#define llMNCharactersDonkeySpecialLwNameSprite ((intptr_t)0x2eec8)
#define llMNCharactersSamusSpecialLwNameSprite ((intptr_t)0x2f008)
#define llMNCharactersLuigiSpecialLwNameSprite ((intptr_t)0x2f148)
#define llMNCharactersLinkSpecialLwNameSprite ((intptr_t)0x2f288)
#define llMNCharactersYoshiSpecialLwNameSprite ((intptr_t)0x2f3c8)
#define llMNCharactersCaptainSpecialLwNameSprite ((intptr_t)0x2f508)
#define llMNCharactersKirbySpecialLwNameSprite ((intptr_t)0x2f648)
#define llMNCharactersPikachuSpecialLwNameSprite ((intptr_t)0x2f788)
#define llMNCharactersPurinSpecialLwNameSprite ((intptr_t)0x2f8c8)
#define llMNCharactersNessSpecialLwNameSprite ((intptr_t)0x2fa08)
#define llMNCharactersStoryWallpaperSprite ((intptr_t)0x30888)
#define llMNPlayersCommonManTextSprite ((intptr_t)0x40)
#define llMNPlayersCommonComTextSprite ((intptr_t)0xd0)
#define llMNPlayersCommonColon0TextSprite ((intptr_t)0x160)
#define llMNPlayersCommonColon1TextSprite ((intptr_t)0x1f0)
#define llMNPlayersCommonColon2TextSprite ((intptr_t)0x280)
#define llMNPlayersCommonColon3TextSprite ((intptr_t)0x310)
#define llMNPlayersCommonColon4TextSprite ((intptr_t)0x3a0)
#define llMNPlayersCommonPlayerTextSprite ((intptr_t)0x458)
#define llMNPlayersCommon1TextSprite ((intptr_t)0x4e0)
#define llMNPlayersCommon2TextSprite ((intptr_t)0x570)
#define llMNPlayersCommon3TextSprite ((intptr_t)0x600)
#define llMNPlayersCommon4TextSprite ((intptr_t)0x690)
#define llMNPlayersCommon1PTextSprite ((intptr_t)0x878)
#define llMNPlayersCommon2PTextSprite ((intptr_t)0xa58)
#define llMNPlayersCommon3PTextSprite ((intptr_t)0xc38)
#define llMNPlayersCommon4PTextSprite ((intptr_t)0xe18)
#define llMNPlayersCommonCPTextSprite ((intptr_t)0xff8)
#define llMNPlayersCommonHandicapTextSprite ((intptr_t)0x1108)
#define llMNPlayersCommonCPLevelTextSprite ((intptr_t)0x1218)
#define llMNPlayersCommonPushTextSprite ((intptr_t)0x12c8)
#define llMNPlayersCommonStartTextSprite ((intptr_t)0x1378)
#define llMNPlayersCommonButtonTextSprite ((intptr_t)0x1428)
#define llMNPlayersCommonPressTextSprite ((intptr_t)0x14d8)
#define llMNPlayersCommonMarioTextSprite ((intptr_t)0x1838)
#define llMNPlayersCommonLuigiTextSprite ((intptr_t)0x1b18)
#define llMNPlayersCommonDKTextSprite ((intptr_t)0x1ff8)
#define llMNPlayersCommonSamusTextSprite ((intptr_t)0x2358)
#define llMNPlayersCommonFoxTextSprite ((intptr_t)0x25b8)
#define llMNPlayersCommonKirbyTextSprite ((intptr_t)0x28e8)
#define llMNPlayersCommonLinkTextSprite ((intptr_t)0x2ba0)
#define llMNPlayersCommonYoshiTextSprite ((intptr_t)0x2ed8)
#define llMNPlayersCommonPikachuTextSprite ((intptr_t)0x32f8)
#define llMNPlayersCommonNessTextSprite ((intptr_t)0x35b0)
#define llMNPlayersCommonCaptainFalconTextSprite ((intptr_t)0x3998)
#define llMNPlayersCommonJigglypuffTextSprite ((intptr_t)0x3db8)
#define llMNPlayersCommonInfinityDarkSprite ((intptr_t)0x3ef0)
#define llMNPlayersCommonTimeSelectorSprite ((intptr_t)0x48b0)
#define llMNPlayersCommonStockSelectorSprite ((intptr_t)0x5270)
#define llMNPlayersCommon0DarkSprite ((intptr_t)0x5388)
#define llMNPlayersCommon1DarkSprite ((intptr_t)0x5440)
#define llMNPlayersCommon2DarkSprite ((intptr_t)0x5558)
#define llMNPlayersCommon3DarkSprite ((intptr_t)0x5668)
#define llMNPlayersCommon4DarkSprite ((intptr_t)0x5778)
#define llMNPlayersCommon5DarkSprite ((intptr_t)0x5888)
#define llMNPlayersCommon6DarkSprite ((intptr_t)0x5998)
#define llMNPlayersCommon7DarkSprite ((intptr_t)0x5aa8)
#define llMNPlayersCommon8DarkSprite ((intptr_t)0x5bb8)
#define llMNPlayersCommon9DarkSprite ((intptr_t)0x5cc8)
#define llMNPlayersCommonHmnLabelSprite ((intptr_t)0x6048)
#define llMNPlayersCommonCPLabelSprite ((intptr_t)0x63c8)
#define llMNPlayersCommonNALabelSprite ((intptr_t)0x6748)
#define llMNPlayersCommonCursorHandPointSprite ((intptr_t)0x6f88)
#define llMNPlayersCommonCursorHandGrabSprite ((intptr_t)0x76e8)
#define llMNPlayersCommonCursorHandHoverSprite ((intptr_t)0x8168)
#define llMNPlayersCommon1PTextGradientSprite ((intptr_t)0x8268)
#define llMNPlayersCommon2PTextGradientSprite ((intptr_t)0x8368)
#define llMNPlayersCommon3PTextGradientSprite ((intptr_t)0x8468)
#define llMNPlayersCommon4PTextGradientSprite ((intptr_t)0x8568)
#define llMNPlayersCommon1PPuckSprite ((intptr_t)0x9048)
#define llMNPlayersCommon2PPuckSprite ((intptr_t)0x9b28)
#define llMNPlayersCommon3PPuckSprite ((intptr_t)0xa608)
#define llMNPlayersCommon4PPuckSprite ((intptr_t)0xb0e8)
#define llMNPlayersCommonCPPuckSprite ((intptr_t)0xbbc8)
#define llMNPlayersCommonSmashLogoCardLeftSprite ((intptr_t)0xcdb0)
#define llMNPlayersCommonSmashLogoCardRightSprite ((intptr_t)0xdfa0)
#define llMNPlayersCommonRedLabelSprite ((intptr_t)0xe3c8)
#define llMNPlayersCommonGreenLabelSprite ((intptr_t)0xe7e8)
#define llMNPlayersCommonBlueLabelSprite ((intptr_t)0xec08)
#define llMNPlayersCommonArrowLSprite ((intptr_t)0xece8)
#define llMNPlayersCommonArrowRSprite ((intptr_t)0xedc8)
#define llMNPlayersCommonReadyToFightTextSprite ((intptr_t)0xf448)
#define llMNPlayersCommonReadyBannerSprite ((intptr_t)0xf530)
#define llMNPlayersCommonGateMan1PLUT ((intptr_t)0x103F8)
#define llMNPlayersCommonGateMan2PLUT ((intptr_t)0x10420)
#define llMNPlayersCommonGateMan4PLUT ((intptr_t)0x10448)
#define llMNPlayersCommonGateMan3PLUT ((intptr_t)0x10470)
#define llMNPlayersCommonGateCom1PLUT ((intptr_t)0x11378)
#define llMNPlayersCommonGateCom2PLUT ((intptr_t)0x113A0)
#define llMNPlayersCommonGateCom4PLUT ((intptr_t)0x113C8)
#define llMNPlayersCommonGateCom3PLUT ((intptr_t)0x113F0)
#define llMNPlayersCommonRedCardSprite ((intptr_t)0x104b0)
#define llMNPlayersCommonGrayCardSprite ((intptr_t)0x11430)
#define llMNPlayersCommonBackButtonSprite ((intptr_t)0x115c8)
#define llMNPlayersGameModesFreeForAllTextSprite ((intptr_t)0x280)
#define llMNPlayersGameModesTeamBattleTextSprite ((intptr_t)0x4e0)
#define llMNPlayersGameModesTrainingModeTextSprite ((intptr_t)0x758)
#define llMNPlayersGameModesBonus1BreakTheTargetsTextSprite ((intptr_t)0xbd8)
#define llMNPlayersGameModesBonus2BoardThePlatformsTextSprite ((intptr_t)0x1058)
#define llMNPlayersPortraitsCrossSprite ((intptr_t)0x2B8)
#define llMNPlayersPortraitsWhiteSquareSprite ((intptr_t)0x6F0)
#define llMNPlayersPortraitsPortraitQuestionMarkSprite ((intptr_t)0xF68)
#define llMNPlayersPortraitsBlackSquareSprite ((intptr_t)0x13d0)
#define llMNPlayersPortraitsPortraitFireBgSprite ((intptr_t)0x24D0)
#define llMNPlayersPortraitsMarioSprite ((intptr_t)0x4728)
#define llMNPlayersPortraitsLuigiSprite ((intptr_t)0x6978)
#define llMNPlayersPortraitsDonkeySprite ((intptr_t)0x8bc8)
#define llMNPlayersPortraitsSamusSprite ((intptr_t)0xae18)
#define llMNPlayersPortraitsFoxSprite ((intptr_t)0xd068)
#define llMNPlayersPortraitsKirbySprite ((intptr_t)0xf2b8)
#define llMNPlayersPortraitsLinkSprite ((intptr_t)0x11508)
#define llMNPlayersPortraitsYoshiSprite ((intptr_t)0x13758)
#define llMNPlayersPortraitsPikachuSprite ((intptr_t)0x159a8)
#define llMNPlayersPortraitsNessSprite ((intptr_t)0x17bf8)
#define llMNPlayersPortraitsCaptainSprite ((intptr_t)0x19e48)
#define llMNPlayersPortraitsPurinSprite ((intptr_t)0x1c098)
#define llMNPlayersPortraitsCaptainShadowSprite ((intptr_t)0x1e2e8)
#define llMNPlayersPortraitsLuigiShadowSprite ((intptr_t)0x20538)
#define llMNPlayersPortraitsNessShadowSprite ((intptr_t)0x22788)
#define llMNPlayersPortraitsPurinShadowSprite ((intptr_t)0x249d8)
#define llFTEmblemSpritesMarioSprite ((intptr_t)0x618)
#define llFTEmblemSpritesDonkeySprite ((intptr_t)0xc78)
#define llFTEmblemSpritesMetroidSprite ((intptr_t)0x12d8)
#define llFTEmblemSpritesFoxSprite ((intptr_t)0x1938)
#define llFTEmblemSpritesKirbySprite ((intptr_t)0x1f98)
#define llFTEmblemSpritesZeldaSprite ((intptr_t)0x25f8)
#define llFTEmblemSpritesYoshiSprite ((intptr_t)0x2c58)
#define llFTEmblemSpritesFZeroSprite ((intptr_t)0x32b8)
#define llFTEmblemSpritesPMonstersSprite ((intptr_t)0x3918)
#define llFTEmblemSpritesMotherSprite ((intptr_t)0x3f78)
#define llMNSelectCommonPlayersSpotlightMObjSub ((intptr_t)0x408)
#define llMNSelectCommonStoneBackgroundSprite ((intptr_t)0x440)
#define llMNSelectCommonPlayersSpotlightDObjDesc ((intptr_t)0x568)
#define llMNPlayersSpotlightMObjSub ((intptr_t)0x408)
#define llMNPlayersSpotlightDObjDesc ((intptr_t)0x568)
#define llMNPlayers1PMode1PlayerGameTextSprite ((intptr_t)0x228)
#define llMNPlayers1PModeClosingParenthesisSprite ((intptr_t)0x2c8)
#define llMNPlayers1PModeOpeningParenthesisSprite ((intptr_t)0x368)
#define llMNPlayers1PModeLevelColonTextSprite ((intptr_t)0x488)
#define llMNPlayers1PModeStockColonTextSprite ((intptr_t)0x5a8)
#define llMNPlayers1PModeOptionOutlineSprite ((intptr_t)0x1208)
#define llMNPlayers1PModeBestTimeTextSprite ((intptr_t)0x12e0)
#define llMNPlayers1PModeTotalBestTimeTextSprite ((intptr_t)0x1410)
#define llMNPlayers1PModeTargetsTextSprite ((intptr_t)0x1658)
#define llMNPlayers1PModePlatformsTextSprite ((intptr_t)0x1898)
#define llMNPlayers1PModeSmashLogoSprite ((intptr_t)0x1950)
#define llMNPlayers1PModeOptionTextSprite ((intptr_t)0x1ec8)
#define llMNPlayers1PModeSecSprite ((intptr_t)0x1f48)
#define llMNPlayers1PModeCSecSprite ((intptr_t)0x1fc8)
#define llMNPlayers1PModeGateCPLUT ((intptr_t)0x3238)
#define llMNPlayers1PModeRedCardSprite ((intptr_t)0x32a8)
#define llMNPlayersDifficultyEasyTextSprite ((intptr_t)0x98)
#define llMNPlayersDifficultyHardTextSprite ((intptr_t)0x178)
#define llMNPlayersDifficultyNormalTextSprite ((intptr_t)0x2d8)
#define llMNPlayersDifficultyVeryEasyTextSprite ((intptr_t)0x438)
#define llMNPlayersDifficultyVeryHardTextSprite ((intptr_t)0x598)
#define llFTStocksZakoSprite ((intptr_t)0x80)
#define llGRWallpaperTrainingBlackSprite ((intptr_t)0x20718)
#define llGRWallpaperTrainingYellowSprite ((intptr_t)0x20718)
#define llGRWallpaperTrainingBlueSprite ((intptr_t)0x20718)
#define ll_29_Digit0Sprite ((intptr_t)0x98)
#define ll_29_Digit1Sprite ((intptr_t)0x178)
#define ll_29_Digit2Sprite ((intptr_t)0x2d8)
#define ll_29_Digit3Sprite ((intptr_t)0x3b8)
#define ll_29_Digit4Sprite ((intptr_t)0x518)
#define ll_29_Digit5Sprite ((intptr_t)0x678)
#define ll_29_Digit6Sprite ((intptr_t)0x7d8)
#define ll_29_Digit7Sprite ((intptr_t)0x938)
#define ll_29_Digit8Sprite ((intptr_t)0xa98)
#define ll_29_Digit9Sprite ((intptr_t)0xbf8)
#define ll_29_NoneTextSprite ((intptr_t)0xe58)
#define ll_29_MaximTomatoTextSprite ((intptr_t)0x14b8)
#define ll_29_HeartTextSprite ((intptr_t)0x1798)
#define ll_29_StarTextSprite ((intptr_t)0x19f8)
#define ll_29_BeamSwordTextSprite ((intptr_t)0x1f58)
#define ll_29_HomerunBatTextSprite ((intptr_t)0x24b8)
#define ll_29_FanTextSprite ((intptr_t)0x2698)
#define ll_29_StarRodTextSprite ((intptr_t)0x2af8)
#define ll_29_RayGunTextSprite ((intptr_t)0x2e58)
#define ll_29_FireFlowerTextSprite ((intptr_t)0x33b8)
#define ll_29_HammerTextSprite ((intptr_t)0x3798)
#define ll_29_MotionSensorBombSprite ((intptr_t)0x3ff8)
#define ll_29_BobOmbTextSprite ((intptr_t)0x43d8)
#define ll_29_BumperTextSprite ((intptr_t)0x4738)
#define ll_29_GreenShellTextSprite ((intptr_t)0x4c98)
#define ll_29_RedShellTextSprite ((intptr_t)0x50f8)
#define ll_29_PokeballTextSprite ((intptr_t)0x5558)
#define ll_29_OneSlashOneTextSprite ((intptr_t)0x5738)
#define ll_29_OneSlashTwoTextSprite ((intptr_t)0x5918)
#define ll_29_TwoSlashThreeTextSprite ((intptr_t)0x5b78)
#define ll_29_OneSlashFourTextSprite ((intptr_t)0x5d58)
#define ll_29_StandTextSprite ((intptr_t)0x6038)
#define ll_29_WalkTextSprite ((intptr_t)0x6318)
#define ll_29_EvadeTextSprite ((intptr_t)0x65f8)
#define ll_29_JumpTextSprite ((intptr_t)0x68d8)
#define ll_29_AttackTextSprite ((intptr_t)0x6c38)
#define ll_29_OpeningBracketSprite ((intptr_t)0x6d18)
#define ll_29_ClosingBracketSprite ((intptr_t)0x6df8)
#define ll_29_PercentageSignSprite ((intptr_t)0x6f58)
#define ll_29_DamageColonTextSprite ((intptr_t)0x7338)
#define ll_29_ComboColonTextSprite ((intptr_t)0x7698)
#define ll_29_EnemyColonTextSprite ((intptr_t)0x79f8)
#define ll_29_SpeedColonTextSprite ((intptr_t)0x7cd8)
#define ll_29_NoneText2Sprite ((intptr_t)0x7f90)
#define ll_29_MaximTomatoText2Sprite ((intptr_t)0x85f8)
#define ll_29_HeartText2Sprite ((intptr_t)0x88d8)
#define ll_29_StarText2Sprite ((intptr_t)0x8b38)
#define ll_29_BeamSwordText2Sprite ((intptr_t)0x9048)
#define ll_29_HomerunBatText2Sprite ((intptr_t)0x9558)
#define ll_29_FanText2Sprite ((intptr_t)0x9720)
#define ll_29_StarRodText2Sprite ((intptr_t)0x9b08)
#define ll_29_RayGunText2Sprite ((intptr_t)0x9e38)
#define ll_29_FireFlowerText2Sprite ((intptr_t)0xa318)
#define ll_29_HammerText2Sprite ((intptr_t)0xa6f8)
#define ll_29_MotionSensorBomb2Sprite ((intptr_t)0xad30)
#define ll_29_BobOmbText2Sprite ((intptr_t)0xb158)
#define ll_29_BumperText2Sprite ((intptr_t)0xb538)
#define ll_29_GreenShellText2Sprite ((intptr_t)0xba18)
#define ll_29_RedShellText2Sprite ((intptr_t)0xbdf8)
#define ll_29_PokeballText2Sprite ((intptr_t)0xc1a0)
#define ll_29_OneSlashOneText2Sprite ((intptr_t)0xc388)
#define ll_29_OneSlashTwoText2Sprite ((intptr_t)0xc568)
#define ll_29_TwoSlashThreeText2Sprite ((intptr_t)0xc748)
#define ll_29_OneSlashFourText2Sprite ((intptr_t)0xc928)
#define ll_29_StandText2Sprite ((intptr_t)0xcc08)
#define ll_29_WalkText2Sprite ((intptr_t)0xcee8)
#define ll_29_EvadeText2Sprite ((intptr_t)0xd1a0)
#define ll_29_JumpText2Sprite ((intptr_t)0xd488)
#define ll_29_AttackText2Sprite ((intptr_t)0xd7e8)
#define ll_29_CPColonTextSprite ((intptr_t)0xdb90)
#define ll_29_ExitTextSprite ((intptr_t)0xdec8)
#define ll_29_ItemColonTextSprite ((intptr_t)0xe270)
#define ll_29_RedSphereSprite ((intptr_t)0xe6d8)
#define ll_29_ResetTextSprite ((intptr_t)0xea08)
#define ll_29_SpeedTextSprite ((intptr_t)0xedb0)
#define ll_29_ViewColonTextSprite ((intptr_t)0xf160)
#define ll_29_CloseUpTextSprite ((intptr_t)0xf588)
#define ll_29_NormalTextSprite ((intptr_t)0xf8e8)
#define ll_29_BoxTopSprite ((intptr_t)0xfb10)
#define ll_29_BoxLeftSprite ((intptr_t)0x10060)
#define ll_29_BoxRightSprite ((intptr_t)0x105b0)
#define ll_29_BoxBottomSprite ((intptr_t)0x107e0)
#define ll_29_ArrowLSprite ((intptr_t)0x10890)
#define ll_29_ArrowRSprite ((intptr_t)0x10940)
#define ll_29_NoneTextJapSprite ((intptr_t)0x10a68)
#define ll_29_MaximTomatoTextJapSprite ((intptr_t)0x10ca8)
#define ll_29_HeartTextJapSprite ((intptr_t)0x10ee8)
#define ll_29_StarTextJapSprite ((intptr_t)0x11068)
#define ll_29_BeamSwordTextJapSprite ((intptr_t)0x11248)
#define ll_29_HomerunBatTextJapSprite ((intptr_t)0x11488)
#define ll_29_FanTextJapSprite ((intptr_t)0x11608)
#define ll_29_StarRodTextJapSprite ((intptr_t)0x11848)
#define ll_29_RayGunTextJapSprite ((intptr_t)0x119c8)
#define ll_29_FireFlowerTextJapSprite ((intptr_t)0x11c08)
#define ll_29_HammerTextJapSprite ((intptr_t)0x11d88)
#define ll_29_MotionSensorBombJapSprite ((intptr_t)0x12048)
#define ll_29_BobOmbTextJapSprite ((intptr_t)0x121c8)
#define ll_29_BumperTextJapSprite ((intptr_t)0x12348)
#define ll_29_GreenShellTextJapSprite ((intptr_t)0x12588)
#define ll_29_RedShellTextJapSprite ((intptr_t)0x12768)
#define ll_29_PokeballTextJapSprite ((intptr_t)0x129a8)
#define ll_29_OneSlashOneTextJapSprite ((intptr_t)0x12ac8)
#define ll_29_OneSlashTwoTextJapSprite ((intptr_t)0x12be8)
#define ll_29_TwoSlashThreeTextJapSprite ((intptr_t)0x12d08)
#define ll_29_OneSlashFourTextJapSprite ((intptr_t)0x12e28)
#define ll_29_StandTextJapSprite ((intptr_t)0x12f48)
#define ll_29_WalkTextJapSprite ((intptr_t)0x13068)
#define ll_29_EvadeTextJapSprite ((intptr_t)0x131e8)
#define ll_29_JumpTextJapSprite ((intptr_t)0x13368)
#define ll_29_AttackTextJapSprite ((intptr_t)0x13488)
#define ll_29_EnemyStatusTextJapSprite ((intptr_t)0x13668)
#define ll_29_QuitTextJapSprite ((intptr_t)0x13968)
#define ll_29_ItemTextJapSprite ((intptr_t)0x13ae8)
#define ll_29_ResetTextJapSprite ((intptr_t)0x13c68)
#define ll_29_SlowTextJapSprite ((intptr_t)0x13de8)
#define ll_29_CameraTextJapSprite ((intptr_t)0x13f68)
#define ll_29_NearTextJapSprite ((intptr_t)0x14088)
#define ll_29_NormalTextJapSprite ((intptr_t)0x141a8)
#define llMNMapsPeachsCastleTextSprite ((intptr_t)0x1f8)
#define llMNMapsSectorZTextSprite ((intptr_t)0x438)
#define llMNMapsCongoJungleTextSprite ((intptr_t)0x678)
#define llMNMapsPlanetZebesTextSprite ((intptr_t)0x8b8)
#define llMNMapsHyruleCastleTextSprite ((intptr_t)0xb10)
#define llMNMapsYoshisIslandTextSprite ((intptr_t)0xd58)
#define llMNMapsSaffronCityTextSprite ((intptr_t)0xf98)
#define llMNMapsMushroomKingdomTextSprite ((intptr_t)0x11d8)
#define llMNMapsDreamLandTextSprite ((intptr_t)0x1418)
#define llMNMapsCursorSprite ((intptr_t)0x1ab8)
#define llMNMapsQuestionMarkSprite ((intptr_t)0x1dd8)
#define llMNMapsStageSelectTextSprite ((intptr_t)0x26a0)
#define llMNMapsWoodenCircleSprite ((intptr_t)0x3840)
#define llMNMapsPlateRightSprite ((intptr_t)0x3c68)
#define llMNMapsPlateMiddleSprite ((intptr_t)0x3d68)
#define llMNMapsPlateLeftSprite ((intptr_t)0x3fa8)
#define llMNMapsPeachsCastleSprite ((intptr_t)0x4d88)
#define llMNMapsSectorZSprite ((intptr_t)0x5b68)
#define llMNMapsCongoJungleSprite ((intptr_t)0x6948)
#define llMNMapsPlanetZebesSprite ((intptr_t)0x7728)
#define llMNMapsHyruleCastleSprite ((intptr_t)0x8508)
#define llMNMapsYoshisIslandSprite ((intptr_t)0x92e8)
#define llMNMapsSaffronCitySprite ((intptr_t)0xa0c8)
#define llMNMapsMushroomKingdomSprite ((intptr_t)0xaea8)
#define llMNMapsDreamLandSprite ((intptr_t)0xbc88)
#define llMNMapsTilesSprite ((intptr_t)0xc728)
#define llMNMapsRandomSmallSprite ((intptr_t)0xcb10)
#define llMNMapsRandomBigSprite ((intptr_t)0xde30)
#define llMNVSRecordMainQuestionSprite ((intptr_t)0x70)
#define llMNVSRecordMainUnknownSprite ((intptr_t)0x168)
#define llMNVSRecordMainLabelTotalSprite ((intptr_t)0x258)
#define llMNVSRecordMainDigit0Sprite ((intptr_t)0x2f0)
#define llMNVSRecordMainDigit1Sprite ((intptr_t)0x390)
#define llMNVSRecordMainDigit2Sprite ((intptr_t)0x430)
#define llMNVSRecordMainDigit3Sprite ((intptr_t)0x4d0)
#define llMNVSRecordMainDigit4Sprite ((intptr_t)0x570)
#define llMNVSRecordMainDigit5Sprite ((intptr_t)0x610)
#define llMNVSRecordMainDigit6Sprite ((intptr_t)0x6b0)
#define llMNVSRecordMainDigit7Sprite ((intptr_t)0x750)
#define llMNVSRecordMainDigit8Sprite ((intptr_t)0x7f0)
#define llMNVSRecordMainDigit9Sprite ((intptr_t)0x890)
#define llMNVSRecordMainSymbolPointSprite ((intptr_t)0x910)
#define llMNVSRecordMainLabelWinPercentSprite ((intptr_t)0xa08)
#define llMNVSRecordMainLabelKOsSprite ((intptr_t)0xaf8)
#define llMNVSRecordMainLabelTKOSprite ((intptr_t)0xbe8)
#define llMNVSRecordMainLabelSDPercentSprite ((intptr_t)0xcd8)
#define llMNVSRecordMainLabelTimeSprite ((intptr_t)0xe10)
#define llMNVSRecordMainLabelUsePercentSprite ((intptr_t)0xf08)
#define llMNVSRecordMainLabelAvgSprite ((intptr_t)0x1008)
#define llMNVSRecordMainLabelKOdSprite ((intptr_t)0x1140)
#define llMNVSRecordMainSymbolSlashSprite ((intptr_t)0x11d0)
#define llMNVSRecordMainUnknown1Sprite ((intptr_t)0x1318)
#define llMNVSRecordMainUnknown2Sprite ((intptr_t)0x1458)
#define llMNVSRecordMainBattleScoreSprite ((intptr_t)0x15d0)
#define llMNVSRecordMainDownArrowsSprite ((intptr_t)0x1668)
#define llMNVSRecordMainSideArrowsSprite ((intptr_t)0x17a8)
#define llMNVSRecordMainMarioIconBWSprite ((intptr_t)0x1918)
#define llMNVSRecordMainFoxIconBWSprite ((intptr_t)0x1a98)
#define llMNVSRecordMainDonkeyIconBWSprite ((intptr_t)0x1ca8)
#define llMNVSRecordMainSamusIconBWSprite ((intptr_t)0x1e88)
#define llMNVSRecordMainLuigiIconBWSprite ((intptr_t)0x2008)
#define llMNVSRecordMainYoshiIconBWSprite ((intptr_t)0x2178)
#define llMNVSRecordMainLinkIconBWSprite ((intptr_t)0x2370)
#define llMNVSRecordMainCaptainIconBWSprite ((intptr_t)0x2540)
#define llMNVSRecordMainNessIconBWSprite ((intptr_t)0x2698)
#define llMNVSRecordMainPurinIconBWSprite ((intptr_t)0x27c8)
#define llMNVSRecordMainKirbyIconBWSprite ((intptr_t)0x2930)
#define llMNVSRecordMainPikachuIconBWSprite ((intptr_t)0x2b30)
#define llMNVSRecordMainMarioIconColorSprite ((intptr_t)0x2d18)
#define llMNVSRecordMainFoxIconColorSprite ((intptr_t)0x2ef8)
#define llMNVSRecordMainDonkeyIconColorSprite ((intptr_t)0x3198)
#define llMNVSRecordMainSamusIconColorSprite ((intptr_t)0x3438)
#define llMNVSRecordMainLuigiIconColorSprite ((intptr_t)0x3618)
#define llMNVSRecordMainYoshiIconColorSprite ((intptr_t)0x37f8)
#define llMNVSRecordMainLinkIconColorSprite ((intptr_t)0x3a38)
#define llMNVSRecordMainCaptainIconColorSprite ((intptr_t)0x3cd8)
#define llMNVSRecordMainNessIconColorSprite ((intptr_t)0x3eb8)
#define llMNVSRecordMainPurinIconColorSprite ((intptr_t)0x4098)
#define llMNVSRecordMainKirbyIconColorSprite ((intptr_t)0x4308)
#define llMNVSRecordMainPikachuIconColorSprite ((intptr_t)0x45a8)
#define llMNVSRecordMainPortraitWallpaperSprite ((intptr_t)0x4d30)
#define llMNVSRecordMainLabelSprite ((intptr_t)0x5428)
#define llMNVSRecordMainSymbolColonSprite ((intptr_t)0x54c0)
#define llMNDataCommonDataHeaderSprite ((intptr_t)0xb40)
#define llMNDataCommonArrowLSprite ((intptr_t)0xbe0)
#define llMNDataCommonArrowRSprite ((intptr_t)0xc80)
#define llMNCommonFontsLetterASprite ((intptr_t)0x40)
#define llMNCommonFontsLetterBSprite ((intptr_t)0xd0)
#define llMNCommonFontsLetterCSprite ((intptr_t)0x160)
#define llMNCommonFontsLetterDSprite ((intptr_t)0x1f0)
#define llMNCommonFontsLetterESprite ((intptr_t)0x280)
#define llMNCommonFontsLetterFSprite ((intptr_t)0x310)
#define llMNCommonFontsLetterGSprite ((intptr_t)0x3a0)
#define llMNCommonFontsLetterHSprite ((intptr_t)0x430)
#define llMNCommonFontsLetterISprite ((intptr_t)0x4c0)
#define llMNCommonFontsLetterJSprite ((intptr_t)0x550)
#define llMNCommonFontsLetterKSprite ((intptr_t)0x5e0)
#define llMNCommonFontsLetterLSprite ((intptr_t)0x670)
#define llMNCommonFontsLetterMSprite ((intptr_t)0x700)
#define llMNCommonFontsLetterNSprite ((intptr_t)0x790)
#define llMNCommonFontsLetterOSprite ((intptr_t)0x820)
#define llMNCommonFontsLetterPSprite ((intptr_t)0x8b0)
#define llMNCommonFontsLetterQSprite ((intptr_t)0x940)
#define llMNCommonFontsLetterRSprite ((intptr_t)0x9d0)
#define llMNCommonFontsLetterSSprite ((intptr_t)0xa60)
#define llMNCommonFontsLetterTSprite ((intptr_t)0xaf0)
#define llMNCommonFontsLetterUSprite ((intptr_t)0xb80)
#define llMNCommonFontsLetterVSprite ((intptr_t)0xc10)
#define llMNCommonFontsLetterWSprite ((intptr_t)0xca0)
#define llMNCommonFontsLetterXSprite ((intptr_t)0xd30)
#define llMNCommonFontsLetterYSprite ((intptr_t)0xdc0)
#define llMNCommonFontsLetterZSprite ((intptr_t)0xe50)
#define llMNCommonFontsSymbolApostropheSprite ((intptr_t)0xed0)
#define llMNCommonFontsSymbolPercentSprite ((intptr_t)0xf60)
#define llMNCommonFontsSymbolPeriodSprite ((intptr_t)0xfd0)
#define llMNVSResultsTKOTextSprite ((intptr_t)0x358)
#define llMNVSResultsPlaceTextSprite ((intptr_t)0x990)
#define llMNVSResultsKOsTextSprite ((intptr_t)0xd38)
#define llMNVSResultsPtsTextSprite ((intptr_t)0x10d8)
#define llMNVSResults0x48c8Sprite ((intptr_t)0x48c8)
#define llMNVSResults1PArrowSprite ((intptr_t)0x49e8)
#define llMNVSResults2PArrowSprite ((intptr_t)0x4b08)
#define llMNVSResults3PArrowSprite ((intptr_t)0x4c28)
#define llMNVSResults4PArrowSprite ((intptr_t)0x4d48)
#define llMNVSResultsWallpaperSprite ((intptr_t)0xd5c8)
#define llMNVSResultsWinnerSprite ((intptr_t)0xe2a0)
#define llFTEmblemModelsMarioMObjSub ((intptr_t)0x0000)
#define llFTEmblemModelsMarioDObjDesc ((intptr_t)0x0990)
#define llFTEmblemModelsMarioMatAnimJoint ((intptr_t)0x0A14)
#define llFTEmblemModelsFoxMObjSub ((intptr_t)0x1940)
#define llFTEmblemModelsFoxDObjDesc ((intptr_t)0x21D0)
#define llFTEmblemModelsFoxMatAnimJoint ((intptr_t)0x2254)
#define llFTEmblemModelsDonkeyMObjSub ((intptr_t)0x0B00)
#define llFTEmblemModelsDonkeyDObjDesc ((intptr_t)0x1348)
#define llFTEmblemModelsDonkeyMatAnimJoint ((intptr_t)0x13CC)
#define llFTEmblemModelsMetroidMObjSub ((intptr_t)0x1470)
#define llFTEmblemModelsMetroidDObjDesc ((intptr_t)0x1860)
#define llFTEmblemModelsMetroidMatAnimJoint ((intptr_t)0x18E4)
#define llFTEmblemModelsZeldaMObjSub ((intptr_t)0x22B0)
#define llFTEmblemModelsZeldaDObjDesc ((intptr_t)0x2520)
#define llFTEmblemModelsZeldaMatAnimJoint ((intptr_t)0x25A4)
#define llFTEmblemModelsYoshiMObjSub ((intptr_t)0x2690)
#define llFTEmblemModelsYoshiDObjDesc ((intptr_t)0x2F10)
#define llFTEmblemModelsYoshiMatAnimJoint ((intptr_t)0x2F94)
#define llFTEmblemModelsFZeroMObjSub ((intptr_t)0x2FF0)
#define llFTEmblemModelsFZeroDObjDesc ((intptr_t)0x3828)
#define llFTEmblemModelsFZeroMatAnimJoint ((intptr_t)0x38AC)
#define llFTEmblemModelsKirbyMObjSub ((intptr_t)0x3900)
#define llFTEmblemModelsKirbyDObjDesc ((intptr_t)0x3E68)
#define llFTEmblemModelsKirbyMatAnimJoint ((intptr_t)0x3EEC)
#define llFTEmblemModelsPMonstersMObjSub ((intptr_t)0x3F40)
#define llFTEmblemModelsPMonstersDObjDesc ((intptr_t)0x4710)
#define llFTEmblemModelsPMonstersMatAnimJoint ((intptr_t)0x4794)
#define llFTEmblemModelsMotherMObjSub ((intptr_t)0x4840)
#define llFTEmblemModelsMotherDObjDesc ((intptr_t)0x5A00)
#define llFTEmblemModelsMotherMatAnimJoint ((intptr_t)0x5A84)
#define llIFCommonDigits0Sprite ((intptr_t)0x68)
#define llIFCommonDigits1Sprite ((intptr_t)0x118)
#define llIFCommonDigits2Sprite ((intptr_t)0x1c8)
#define llIFCommonDigits3Sprite ((intptr_t)0x278)
#define llIFCommonDigits4Sprite ((intptr_t)0x328)
#define llIFCommonDigits5Sprite ((intptr_t)0x3d8)
#define llIFCommonDigits6Sprite ((intptr_t)0x488)
#define llIFCommonDigits7Sprite ((intptr_t)0x538)
#define llIFCommonDigits8Sprite ((intptr_t)0x5e8)
#define llIFCommonDigits9Sprite ((intptr_t)0x698)
#define llIFCommonDigitsDashSprite ((intptr_t)0x710)
#define llIFCommonDigitsCrossSprite ((intptr_t)0x828)
#define llIFCommonDigitsColonSprite ((intptr_t)0x8d8)
#define llIFCommonAnnounceCommonLetterASprite ((intptr_t)0x5e0)
#define llIFCommonAnnounceCommonLetterBSprite ((intptr_t)0x9a8)
#define llIFCommonAnnounceCommonLetterCSprite ((intptr_t)0xd80)
#define llIFCommonAnnounceCommonLetterDSprite ((intptr_t)0x1268)
#define llIFCommonAnnounceCommonLetterESprite ((intptr_t)0x1628)
#define llIFCommonAnnounceCommonLetterFSprite ((intptr_t)0x1a00)
#define llIFCommonAnnounceCommonLetterGSprite ((intptr_t)0x1f08)
#define llIFCommonAnnounceCommonLetterHSprite ((intptr_t)0x2408)
#define llIFCommonAnnounceCommonLetterISprite ((intptr_t)0x26b8)
#define llIFCommonAnnounceCommonLetterJSprite ((intptr_t)0x2a90)
#define llIFCommonAnnounceCommonLetterKSprite ((intptr_t)0x2f98)
#define llIFCommonAnnounceCommonLetterLSprite ((intptr_t)0x3358)
#define llIFCommonAnnounceCommonLetterMSprite ((intptr_t)0x3980)
#define llIFCommonAnnounceCommonLetterNSprite ((intptr_t)0x3e88)
#define llIFCommonAnnounceCommonLetterOSprite ((intptr_t)0x44b0)
#define llIFCommonAnnounceCommonLetterPSprite ((intptr_t)0x4890)
#define llIFCommonAnnounceCommonLetterQSprite ((intptr_t)0x4f10)
#define llIFCommonAnnounceCommonLetterRSprite ((intptr_t)0x5418)
#define llIFCommonAnnounceCommonLetterSSprite ((intptr_t)0x57f0)
#define llIFCommonAnnounceCommonLetterTSprite ((intptr_t)0x5bd0)
#define llIFCommonAnnounceCommonLetterUSprite ((intptr_t)0x60d8)
#define llIFCommonAnnounceCommonLetterVSprite ((intptr_t)0x65d8)
#define llIFCommonAnnounceCommonLetterWSprite ((intptr_t)0x6c00)
#define llIFCommonAnnounceCommonLetterXSprite ((intptr_t)0x7108)
#define llIFCommonAnnounceCommonLetterYSprite ((intptr_t)0x7608)
#define llIFCommonAnnounceCommonLetterZSprite ((intptr_t)0x7ae8)
#define llIFCommonAnnounceCommonSymbolExclaimSprite ((intptr_t)0x7d98)
#define llIFCommonAnnounceCommonSymbolPeriodSprite ((intptr_t)0x7e50)
#define llIFCommonPlayerTags1PSprite ((intptr_t)0x258)
#define llIFCommonPlayerTags2PSprite ((intptr_t)0x4f8)
#define llIFCommonPlayerTags3PSprite ((intptr_t)0x798)
#define llIFCommonPlayerTags4PSprite ((intptr_t)0xa38)
#define llIFCommonPlayerTagsCPSprite ((intptr_t)0xcd8)
#define llIFCommonPlayerTagsAllySprite ((intptr_t)0xeb8)
#define llLBTransitionAeroplaneDObjDesc ((intptr_t)0xB3F8)
#define llLBTransitionAeroplaneAnimJoint ((intptr_t)0xB710)
#define llLBTransitionCheckDObjDesc ((intptr_t)0x3E80)
#define llLBTransitionCheckAnimJoint ((intptr_t)0x4038)
#define llLBTransitionGakubuthiDObjDesc ((intptr_t)0x0F98)
#define llLBTransitionGakubuthiAnimJoint ((intptr_t)0x101C)
#define llLBTransitionKannonDObjDesc ((intptr_t)0x1F00)
#define llLBTransitionKannonAnimJoint ((intptr_t)0x1FB0)
#define llLBTransitionStarDObjDesc ((intptr_t)0x2450)
#define llLBTransitionStarAnimJoint ((intptr_t)0x24D4)
#define llLBTransitionSudare1DObjDesc ((intptr_t)0x74A8)
#define llLBTransitionSudare1AnimJoint ((intptr_t)0x7660)
#define llLBTransitionSudare2DObjDesc ((intptr_t)0x3EA0)
#define llLBTransitionSudare2AnimJoint ((intptr_t)0x3F50)
#define llLBTransitionBlockDObjDesc ((intptr_t)0x4E18)
#define llLBTransitionBlockAnimJoint ((intptr_t)0x536C)
#define llLBTransitionRotScaleDObjDesc ((intptr_t)0x0F98)
#define llLBTransitionRotScaleAnimJoint ((intptr_t)0x101C)
#define llLBTransitionCurtainDObjDesc ((intptr_t)0x7AE0)
#define llLBTransitionCurtainAnimJoint ((intptr_t)0x7C98)
#define llLBTransitionCameraDObjDesc ((intptr_t)0x3F90)
#define llLBTransitionCameraAnimJoint ((intptr_t)0x4148)
#define llMVCommonRoomBackgroundMObjSub ((intptr_t)0x042F8)
#define llMVCommonRoomBackgroundDObjDesc ((intptr_t)0x07E98)
#define llMVCommonRoomBackgroundMatAnimJoint ((intptr_t)0x08788)
#define llMVCommonRoomDeskDObjDesc ((intptr_t)0x08DF8)
#define llMVCommonRoomHazeDisplayList ((intptr_t)0x098F8)
#define llMVCommonRoomBooksDObjDesc ((intptr_t)0x0A6F8)
#define llMVCommonRoomBooksAnimJoint ((intptr_t)0x0A7B0)
#define llMVCommonRoomPencilsDObjDesc ((intptr_t)0x0AEB8)
#define llMVCommonRoomPencilsAnimJoint ((intptr_t)0x0AF70)
#define llMVCommonRoomLampDObjDesc ((intptr_t)0x0BDC0)
#define llMVCommonRoomLampAnimJoint ((intptr_t)0x0BEA0)
#define llMVCommonRoomTissuesDisplayList ((intptr_t)0x0C690)
#define llMVCommonRoomTissuesAnimJoint ((intptr_t)0x0C884)
#define llMVCommonRoomLogoMObjSub ((intptr_t)0x1BC60)
#define llMVCommonRoomLogoDObjDesc ((intptr_t)0x1C4A8)
#define llMVCommonRoomLogoMatAnimJoint ((intptr_t)0x1C52C)
#define llMVCommonRoomSnapDObjDesc ((intptr_t)0x1CA68)
#define llMVCommonRoomSnapAnimJoint ((intptr_t)0x1CAF0)
#define llMVCommonRoomCloseUpEffectAirMObjSub ((intptr_t)0x1DCA0)
#define llMVCommonRoomCloseUpEffectAirDObjDesc ((intptr_t)0x1DF28)
#define llMVCommonRoomCloseUpEffectAirMatAnimJoint ((intptr_t)0x1DFD8)
#define llMVCommonRoomCloseUpEffectAirAnimJoint ((intptr_t)0x1E010)
#define llMVCommonRoomCloseUpEffectGroundMObjSub ((intptr_t)0x1F0F8)
#define llMVCommonRoomCloseUpEffectGroundDObjDesc ((intptr_t)0x1F270)
#define llMVCommonRoomCloseUpEffectGroundMatAnimJoint ((intptr_t)0x1F2F4)
#define llMVCommonRoomCloseUpEffectGroundAnimJoint ((intptr_t)0x1F330)
#define llMVCommonRoomBossShadowDisplayList ((intptr_t)0x1F790)
#define llMVCommonRoomBossShadowAnimJoint ((intptr_t)0x1F924)
#define llMVCommonRoomDeskGroundMObjSub ((intptr_t)0x20480)
#define llMVCommonRoomDeskGroundDObjDesc ((intptr_t)0x22440)
#define llMVCommonRoomDeskGroundMatAnimJoint ((intptr_t)0x225CC)
#define llMVCommonRoomSpotlightMObjSub ((intptr_t)0x22C90)
#define llMVCommonRoomSpotlightDisplayList ((intptr_t)0x22E18)
#define llMVCommonRoomSpotlightMatAnimJoint ((intptr_t)0x22F10)
#define llMVCommonRoomOutsideDisplayList ((intptr_t)0x24200)
#define llMVCommonRoomSunlightDisplayList ((intptr_t)0x24708)
#define llMVOpeningPortraitsSet1SamusSprite ((intptr_t)0x9960)
#define llMVOpeningPortraitsSet1MarioSprite ((intptr_t)0x13310)
#define llMVOpeningPortraitsSet1FoxSprite ((intptr_t)0x1ccc0)
#define llMVOpeningPortraitsSet1PikachuSprite ((intptr_t)0x26670)
#define llMVOpeningPortraitsSet1CoverSprite ((intptr_t)0x2b2d0)
#define llMVOpeningPortraitsSet2LinkSprite ((intptr_t)0x9960)
#define llMVOpeningPortraitsSet2KirbySprite ((intptr_t)0x13310)
#define llMVOpeningPortraitsSet2DonkeySprite ((intptr_t)0x1ccc0)
#define llMVOpeningPortraitsSet2YoshiSprite ((intptr_t)0x26670)
#define llMVOpeningRunMarioAnimJoint ((intptr_t)0x0004)
#define llMVOpeningRunFoxAnimJoint ((intptr_t)0x00B4)
#define llMVOpeningRunDonkeyAnimJoint ((intptr_t)0x0124)
#define llMVOpeningRunSamusAnimJoint ((intptr_t)0x0184)
#define llMVOpeningRunLinkAnimJoint ((intptr_t)0x0224)
#define llMVOpeningRunYoshiAnimJoint ((intptr_t)0x0334)
#define llMVOpeningRunKirbyAnimJoint ((intptr_t)0x03A4)
#define llMVOpeningRunPikachuAnimJoint ((intptr_t)0x0484)
#define llMVOpeningRunWallpaperSprite ((intptr_t)0x58a0)
#define llMVOpeningRoomScene1CamAnimJoint ((intptr_t)0x0)
#define llMVOpeningRoomScene2CamAnimJoint ((intptr_t)0x0)
#define llMVOpeningRoomScene3CamAnimJoint ((intptr_t)0x0)
#define llMVOpeningRoomScene4CamAnimJoint ((intptr_t)0x0)
#define llMVOpeningRunMainCamAnimJoint ((intptr_t)0x0)
#define llMVOpeningNewcomers1PurinShowDisplayList ((intptr_t)0x05C28)
#define llMVOpeningNewcomers1PurinAnimJoint ((intptr_t)0x05E44)
#define llMVOpeningNewcomers1LuigiShowDisplayList ((intptr_t)0x1C838)
#define llMVOpeningNewcomers1LuigiAnimJoint ((intptr_t)0x1CE94)
#define llMVOpeningNewcomers1PurinHiddenDisplayList ((intptr_t)0x203A8)
#define llMVOpeningNewcomers1LuigiHiddenDisplayList ((intptr_t)0x28C28)
#define llMVOpeningNewcomers2CaptainShowDisplayList ((intptr_t)0x1C238)
#define llMVOpeningNewcomers2CaptainAnimJoint ((intptr_t)0x1C9D4)
#define llMVOpeningNewcomers2NessShowDisplayList ((intptr_t)0x2A448)
#define llMVOpeningNewcomers2NessAnimJoint ((intptr_t)0x2A864)
#define llMVOpeningNewcomers2CaptainHiddenDisplayList ((intptr_t)0x355C0)
#define llMVOpeningNewcomers2NessHiddenDisplayList ((intptr_t)0x3BAF8)
#define llMVOpeningRoomTransitionOverlayDisplayList ((intptr_t)0x5A0)
#define llMVOpeningRoomTransitionOverlayAnimJoint ((intptr_t)0x714)
#define llMVOpeningRoomTransitionOutlineDisplayList ((intptr_t)0xF40)
#define llMVOpeningRoomTransitionOutlineAnimJoint ((intptr_t)0x11C4)
#define llMVOpeningJungleCamAnimJoint ((intptr_t)0x0)
#define llMVOpeningCommonMarioCamAnimJoint ((intptr_t)0x0)
#define llMVOpeningCommonDonkeyCamAnimJoint ((intptr_t)0x30)
#define llMVOpeningCommonSamusCamAnimJoint ((intptr_t)0x60)
#define llMVOpeningCommonFoxCamAnimJoint ((intptr_t)0x90)
#define llMVOpeningCommonLinkCamAnimJoint ((intptr_t)0xC0)
#define llMVOpeningCommonYoshiCamAnimJoint ((intptr_t)0xF0)
#define llMVOpeningCommonPikachuCamAnimJoint ((intptr_t)0x120)
#define llMVOpeningCommonKirbyCamAnimJoint ((intptr_t)0x150)
#define llMVOpeningClashWallpaperCamAnimJoint ((intptr_t)0x4AB0)
#define llMVOpeningClashWallpaperLLMObjSub ((intptr_t)0x3050)
#define llMVOpeningClashWallpaperLLDisplayList ((intptr_t)0x32A8)
#define llMVOpeningClashWallpaperLLMatAnimJoint ((intptr_t)0x3458)
#define llMVOpeningClashWallpaperLLAnimJoint ((intptr_t)0x36D0)
#define llMVOpeningClashWallpaperLRMObjSub ((intptr_t)0x36E0)
#define llMVOpeningClashWallpaperLRDisplayList ((intptr_t)0x3938)
#define llMVOpeningClashWallpaperLRMatAnimJoint ((intptr_t)0x3AE8)
#define llMVOpeningClashWallpaperLRAnimJoint ((intptr_t)0x3D60)
#define llMVOpeningClashWallpaperULMObjSub ((intptr_t)0x3D70)
#define llMVOpeningClashWallpaperULDisplayList ((intptr_t)0x3FC8)
#define llMVOpeningClashWallpaperULMatAnimJoint ((intptr_t)0x4180)
#define llMVOpeningClashWallpaperULAnimJoint ((intptr_t)0x4400)
#define llMVOpeningClashWallpaperURMObjSub ((intptr_t)0x4410)
#define llMVOpeningClashWallpaperURDisplayList ((intptr_t)0x4668)
#define llMVOpeningClashWallpaperURMatAnimJoint ((intptr_t)0x4820)
#define llMVOpeningClashWallpaperURAnimJoint ((intptr_t)0x4AA0)
#define llMVOpeningYosterNestDObjDesc ((intptr_t)0x9808)
#define llMVOpeningYosterGroundDObjDesc ((intptr_t)0xb990)
#define llMVOpeningYosterGroundAnimJoint ((intptr_t)0xbf70)
#define llMVOpeningYosterCamAnimJoint ((intptr_t)0xc940)
#define llMVOpeningCliffHillsDObjDesc ((intptr_t)0x37A0)
#define llMVOpeningCliffOcarinaDObjDesc ((intptr_t)0x67A0)
#define llMVOpeningCliffOcarinaAnimJoint ((intptr_t)0x6850)
#define llMVOpeningCliffCamAnimJoint ((intptr_t)0x8910)
#define llMVOpeningStandoffGroundDisplayList ((intptr_t)0x1C10)
#define llMVOpeningStandoffLightningMObjSub ((intptr_t)0x6140)
#define llMVOpeningStandoffLightningDObjDesc ((intptr_t)0x6950)
#define llMVOpeningStandoffLightningMatAnimJoint ((intptr_t)0x6BB8)
#define llMVOpeningStandoffLightningAnimJoint ((intptr_t)0x6D60)
#define llMVOpeningStandoffCamAnimJoint ((intptr_t)0x7250)
#define llMVOpeningStandoffWallpaperSprite ((intptr_t)0xb500)
#define llMVOpeningYamabukiLegsDObjDesc ((intptr_t)0x09548)
#define llMVOpeningYamabukiLegsAnimJoint ((intptr_t)0x098c0)
#define llMVOpeningYamabukiLegsShadowDObjDesc ((intptr_t)0x0b2b0)
#define llMVOpeningYamabukiLegsShadowAnimJoint ((intptr_t)0x0b390)
#define llMVOpeningYamabukiMBallDObjDesc ((intptr_t)0x0c9e0)
#define llMVOpeningYamabukiMBallAnimJoint ((intptr_t)0x0cac0)
#define llMVOpeningYamabukiCamAnimJoint ((intptr_t)0x0d330)
#define llMVOpeningYamabukiWallpaperSprite ((intptr_t)0x3ee58)
#define llMVOpeningClashFightersCamAnimJoint ((intptr_t)0x1440)
#define llMVOpeningSectorGreatFoxDObjDesc ((intptr_t)0xD820)
#define llMVOpeningSectorGreatFoxAnimJoint ((intptr_t)0xDA10)
#define llMVOpeningSectorCamAnimJoint ((intptr_t)0xF9A0)
#define llMVOpeningSectorArwing0AnimJoint ((intptr_t)0xE110)
#define llMVOpeningSectorArwing1AnimJoint ((intptr_t)0xE910)
#define llMVOpeningSectorArwing2AnimJoint ((intptr_t)0xF1C0)
#define llMVOpeningSectorCockpitSprite ((intptr_t)0x3cc90)
#define llMVOpeningSectorWallpaperSprite ((intptr_t)0x26c88)
#define llMVOpeningRunCrashMObjSub ((intptr_t)0x2AA8)
#define llMVOpeningRunCrashDObjDesc ((intptr_t)0x35F8)
#define llMVOpeningRunCrashMatAnimJoint ((intptr_t)0x3700)
#define llMVEndingOperatorCamAnimJoint ((intptr_t)0x0)
#define llMNBackupClearHeaderBackupClearSprite ((intptr_t)0xb60)
#define llMNBackupClear0x11e0Sprite ((intptr_t)0x11e0)
#define llMNBackupClear0x1770Sprite ((intptr_t)0x1770)
#define llMNBackupClear0x1b98Sprite ((intptr_t)0x1b98)
#define llMNBackupClear0x2300Sprite ((intptr_t)0x2300)
#define llMNBackupClear0x2890Sprite ((intptr_t)0x2890)
#define llMNBackupClear0x2d30Sprite ((intptr_t)0x2d30)
#define llMNBackupClear0x33b0Sprite ((intptr_t)0x33b0)
#define llMNBackupClearOptionNewcomersSprite ((intptr_t)0x3a00)
#define llMNBackupClearOption1PHighScoreSprite ((intptr_t)0x4050)
#define llMNBackupClearOptionVSRecordSprite ((intptr_t)0x46a0)
#define llMNBackupClearOptionSubjectModeSprite ((intptr_t)0x4cf0)
#define llMNBackupClearOptionPrizeSprite ((intptr_t)0x5340)
#define llMNBackupClearOptionAllDataClearSprite ((intptr_t)0x5990)
#define llMNBackupClearOptionCircleSprite ((intptr_t)0x5db8)
#define llMNBackupClearIsOkayTextSprite ((intptr_t)0x63c8)
#define llMNBackupClearAreYouSureTextSprite ((intptr_t)0x69d8)
#define llMNBackupClearOptionBonusStageTimeSprite ((intptr_t)0x7020)
#define llMNBackupClearOptionYesHighlightPalette ((intptr_t)0x7500)
#define llMNBackupClearOptionYesNotPalette ((intptr_t)0x7528)
#define llMNBackupClearOptionConfirmPalette ((intptr_t)0x7550)
#define llMNBackupClearOptionYesSprite ((intptr_t)0x7580)
#define llMNBackupClearOptionNoHighlightPalette ((intptr_t)0x7a60)
#define llMNBackupClearOptionNoNotPalette ((intptr_t)0x7a88)
#define llMNBackupClearOptionNoSprite ((intptr_t)0x7ab8)
#define llMNBackupClearHeaderOptionSprite ((intptr_t)0xb40)
#define llMN1PContinueContinueTextSprite ((intptr_t)0x18f0)
#define llMN1PContinueYesTextSprite ((intptr_t)0x1e08)
#define llMN1PContinueNoTextSprite ((intptr_t)0x2318)
#define llMN1PContinueCursorSprite ((intptr_t)0x2df8)
#define llMN1PContinueRoomSprite ((intptr_t)0x1e3d8)
#define llMN1PContinueSpotlightSprite ((intptr_t)0x21900)
#define llMN1PContinueShadowSprite ((intptr_t)0x224f8)
#define llSC1PStageClear1StageTextSprite ((intptr_t)0x9d8)
#define llSC1PStageClear1GameTextSprite ((intptr_t)0x1338)
#define llSC1PStageClear1ClearTextSprite ((intptr_t)0x1d58)
#define llSC1PStageClear1SpecialBonusTextSprite ((intptr_t)0x2060)
#define llSC1PStageClear1ColonTextSprite ((intptr_t)0x2120)
#define llSC1PStageClear1TimerTextSprite ((intptr_t)0x25e8)
#define llSC1PStageClear1DamageTextSprite ((intptr_t)0x2b48)
#define llSC1PStageClear10x3028Sprite ((intptr_t)0x3028)
#define llSC1PStageClear10x30f8Sprite ((intptr_t)0x30f8)
#define llSC1PStageClear10x31c8Sprite ((intptr_t)0x31c8)
#define llSC1PStageClear1BonusBorderSprite ((intptr_t)0xa4b8)
#define llSC1PStageClear1ResultTextSprite ((intptr_t)0xaf98)
#define llSC1PStageClear1TargetTextSprite ((intptr_t)0xb4f8)
#define llSC1PStageClear1BonusPageArrowSprite ((intptr_t)0xb6a8)
#define llSC1PStageClear1TimerDamageDigit0Sprite ((intptr_t)0xb808)
#define llSC1PStageClear1TimerDamageDigit1Sprite ((intptr_t)0xb968)
#define llSC1PStageClear1TimerDamageDigit2Sprite ((intptr_t)0xbac8)
#define llSC1PStageClear1TimerDamageDigit3Sprite ((intptr_t)0xbc28)
#define llSC1PStageClear1TimerDamageDigit4Sprite ((intptr_t)0xbd88)
#define llSC1PStageClear1TimerDamageDigit5Sprite ((intptr_t)0xbee8)
#define llSC1PStageClear1TimerDamageDigit6Sprite ((intptr_t)0xc048)
#define llSC1PStageClear1TimerDamageDigit7Sprite ((intptr_t)0xc1a8)
#define llSC1PStageClear1TimerDamageDigit8Sprite ((intptr_t)0xc308)
#define llSC1PStageClear1TimerDamageDigit9Sprite ((intptr_t)0xc468)
#define llSC1PStageClear1TextShadowSprite ((intptr_t)0xd1c8)
#define llSC1PStageClear1BonusTextSprite ((intptr_t)0xd340)
#define llSC1PStageClear1CheapShotTextSprite ((intptr_t)0xd528)
#define llSC1PStageClear1StarFinishTextSprite ((intptr_t)0xd708)
#define llSC1PStageClear1NoItemTextSprite ((intptr_t)0xd8e8)
#define llSC1PStageClear1ShieldBreakerTextSprite ((intptr_t)0xdac8)
#define llSC1PStageClear1JudoWarriorTextSprite ((intptr_t)0xdca8)
#define llSC1PStageClear1HawkTextSprite ((intptr_t)0xde88)
#define llSC1PStageClear1ShooterTextSprite ((intptr_t)0xe068)
#define llSC1PStageClear1HeavyDamageTextSprite ((intptr_t)0xe248)
#define llSC1PStageClear1AllVariationsTextSprite ((intptr_t)0xe428)
#define llSC1PStageClear1ItemStrikeTextSprite ((intptr_t)0xe608)
#define llSC1PStageClear1DoubleKOTextSprite ((intptr_t)0xe7e8)
#define llSC1PStageClear1TricksterTextSprite ((intptr_t)0xe9c8)
#define llSC1PStageClear1GiantImpactTextSprite ((intptr_t)0xeba8)
#define llSC1PStageClear1SpeedsterTextSprite ((intptr_t)0xed88)
#define llSC1PStageClear1ItemThrowTextSprite ((intptr_t)0xef68)
#define llSC1PStageClear1TripleKOTextSprite ((intptr_t)0xf148)
#define llSC1PStageClear1LastChanceTextSprite ((intptr_t)0xf328)
#define llSC1PStageClear1PacifistTextSprite ((intptr_t)0xf508)
#define llSC1PStageClear1PerfectTextSprite ((intptr_t)0xf6e8)
#define llSC1PStageClear1NoMissTextSprite ((intptr_t)0xf8c8)
#define llSC1PStageClear1NoDamageTextSprite ((intptr_t)0xfaa8)
#define llSC1PStageClear1FullPowerTextSprite ((intptr_t)0xfc88)
#define llSC1PStageClear1MewCatcherTextSprite ((intptr_t)0xfe68)
#define llSC1PStageClear1StarClearTextSprite ((intptr_t)0x10048)
#define llSC1PStageClear1VegetarianTextSprite ((intptr_t)0x10228)
#define llSC1PStageClear1HeartThrobTextSprite ((intptr_t)0x10408)
#define llSC1PStageClear1ThrowDownTextSprite ((intptr_t)0x105e8)
#define llSC1PStageClear1SmashManiaTextSprite ((intptr_t)0x107c8)
#define llSC1PStageClear1SmashlessTextSprite ((intptr_t)0x109a8)
#define llSC1PStageClear1SpecialMoveTextSprite ((intptr_t)0x10b88)
#define llSC1PStageClear1SingleMoveTextSprite ((intptr_t)0x10d68)
#define llSC1PStageClear1PokemonFinishTextSprite ((intptr_t)0x10f48)
#define llSC1PStageClear1BoobyTrapTextSprite ((intptr_t)0x11128)
#define llSC1PStageClear1FighterStanceTextSprite ((intptr_t)0x11308)
#define llSC1PStageClear1MysticTextSprite ((intptr_t)0x114e8)
#define llSC1PStageClear1CometMysticTextSprite ((intptr_t)0x116c8)
#define llSC1PStageClear1AcidClearTextSprite ((intptr_t)0x118a8)
#define llSC1PStageClear1BumperClearTextSprite ((intptr_t)0x11a88)
#define llSC1PStageClear1TornadoClearTextSprite ((intptr_t)0x11c68)
#define llSC1PStageClear1ArwingClearTextSprite ((intptr_t)0x11e48)
#define llSC1PStageClear1CounterAttackTextSprite ((intptr_t)0x12028)
#define llSC1PStageClear1MeteorSmashTextSprite ((intptr_t)0x12208)
#define llSC1PStageClear1AerialTextSprite ((intptr_t)0x123e8)
#define llSC1PStageClear1LastSecondTextSprite ((intptr_t)0x125c8)
#define llSC1PStageClear1Lucky3TextSprite ((intptr_t)0x127a8)
#define llSC1PStageClear1JackpotTextSprite ((intptr_t)0x12988)
#define llSC1PStageClear1YoshiRainbowTextSprite ((intptr_t)0x12b68)
#define llSC1PStageClear1KirbyRanksTextSprite ((intptr_t)0x12d48)
#define llSC1PStageClear1BrosCalamityTextSprite ((intptr_t)0x12f28)
#define llSC1PStageClear1DKDefenderTextSprite ((intptr_t)0x13108)
#define llSC1PStageClear1DKPerfectTextSprite ((intptr_t)0x132e8)
#define llSC1PStageClear1GoodFriendTextSprite ((intptr_t)0x134c8)
#define llSC1PStageClear1TrueFriendTextSprite ((intptr_t)0x136a8)
#define llSC1PStageClear1NoMissClearTextSprite ((intptr_t)0x13888)
#define llSC1PStageClear1NoDamageClearTextSprite ((intptr_t)0x13a68)
#define llSC1PStageClear1SpeedKingTextSprite ((intptr_t)0x13c48)
#define llSC1PStageClear1SpeedDemonTextSprite ((intptr_t)0x13e28)
#define llSC1PStageClear1VeryEasyClearTextSprite ((intptr_t)0x14008)
#define llSC1PStageClear1EasyClearTextSprite ((intptr_t)0x141e8)
#define llSC1PStageClear1NormalClearTextSprite ((intptr_t)0x143c8)
#define llSC1PStageClear1HardClearTextSprite ((intptr_t)0x145a8)
#define llSC1PStageClear1VeryHardClearTextSprite ((intptr_t)0x14788)
#define llSC1PStageClear2ScoreTextSprite ((intptr_t)0x408)
#define llIFCommonGameStatusOrangeLetterGSprite ((intptr_t)0x4d78)
#define llIFCommonGameStatusOrangeLetterOSprite ((intptr_t)0xa730)
#define llIFCommonGameStatusOrangeExclamationMarkSprite ((intptr_t)0xc370)
#define llIFCommonGameStatusBlueLetterTSprite ((intptr_t)0xe4a8)
#define llIFCommonGameStatusBlueLetterISprite ((intptr_t)0xf740)
#define llIFCommonGameStatusBlueLetterMSprite ((intptr_t)0x127e0)
#define llIFCommonGameStatusBlueLetterESprite ((intptr_t)0x144e0)
#define llIFCommonGameStatusBlueLetterUSprite ((intptr_t)0x16eb8)
#define llIFCommonGameStatusBlueLetterPSprite ((intptr_t)0x18fe8)
#define llIFCommonGameStatusBlueLetterSSprite ((intptr_t)0x1b5f8)
#define llIFCommonGameStatusBlueLetterASprite ((intptr_t)0x1de68)
#define llIFCommonGameStatusBlueLetterGSprite ((intptr_t)0x20788)
#define llIFCommonGameStatusRodSprite ((intptr_t)0x20990)
#define llIFCommonGameStatusFrameSprite ((intptr_t)0x21760)
#define llIFCommonGameStatusRodShadowSprite ((intptr_t)0x21878)
#define llIFCommonGameStatusLampRedDimSprite ((intptr_t)0x21950)
#define llIFCommonGameStatusLampYellowDimSprite ((intptr_t)0x21a10)
#define llIFCommonGameStatusLampBlueDimSprite ((intptr_t)0x21ba8)
#define llIFCommonGameStatusLampRedLightSprite ((intptr_t)0x22128)
#define llIFCommonGameStatusLampYellowLightSprite ((intptr_t)0x22588)
#define llIFCommonGameStatusLampBlueLightSprite ((intptr_t)0x22f18)
#define llIFCommonGameStatusLampRedContourSprite ((intptr_t)0x23a28)
#define llIFCommonGameStatusLampYellowContourSprite ((intptr_t)0x24620)
#define llIFCommonGameStatusLampBlueContourSprite ((intptr_t)0x25290)
#define llEFCommonEffects1DamageSlashMObjSub ((intptr_t)0x73E0)
#define llEFCommonEffects1DamageSlashDObjDesc ((intptr_t)0x7750)
#define llEFCommonEffects1DamageSlashAnimJoint ((intptr_t)0x7800)
#define llEFCommonEffects1DamageSlashMatAnimJoint ((intptr_t)0x7860)
#define llEFCommonEffects1ImpactWaveMObjSub ((intptr_t)0x7A80)
#define llEFCommonEffects1ImpactWaveDObjDesc ((intptr_t)0x7C28)
#define llEFCommonEffects1ImpactWaveAnimJoint ((intptr_t)0x7D40)
#define llEFCommonEffects1ImpactWaveMatAnimJoint ((intptr_t)0x7DA0)
#define llEFCommonEffects1FlyOrbsDObjDesc ((intptr_t)0x7E80)
#define llEFCommonEffects1FlyOrbsAnimJoint ((intptr_t)0x7F40)
#define llEFCommonEffects1CommonSparkMObjSub ((intptr_t)0x8EC0)
#define llEFCommonEffects1CommonSparkDObjDesc ((intptr_t)0x8FA0)
#define llEFCommonEffects1CommonSparkAnimJoint ((intptr_t)0x9050)
#define llEFCommonEffects1CommonSparkMatAnimJoint ((intptr_t)0x90C0)
#define llEFCommonEffects1DamageFlyMDustMObjSub ((intptr_t)0xC978)
#define llEFCommonEffects1DamageFlyMDustDObjDesc ((intptr_t)0xCAC8)
#define llEFCommonEffects1DamageFlyMDustAnimJoint ((intptr_t)0xCAE0)
#define llEFCommonEffects1DamageFlyMDustMatAnimJoint ((intptr_t)0xCB40)
#define llEFCommonEffects1QuakeMag0AnimJoint ((intptr_t)0xCBC0)
#define llEFCommonEffects1QuakeMag1AnimJoint ((intptr_t)0xCC20)
#define llEFCommonEffects1QuakeMag2AnimJoint ((intptr_t)0xCCF0)
#define llEFCommonEffects1QuakeMag3AnimJoint ((intptr_t)0xCDC0)
#define llEFCommonEffects2ShockSmallMObjSub ((intptr_t)0x1428)
#define llEFCommonEffects2ShockSmallDObjDesc ((intptr_t)0x1500)
#define llEFCommonEffects2ShockSmallMatAnimJoint ((intptr_t)0x1570)
#define llEFCommonEffects2FireSparkMObjSub ((intptr_t)0x1EA0)
#define llEFCommonEffects2FireSparkDObjDesc ((intptr_t)0x2040)
#define llEFCommonEffects2FireSparkAnimJoint ((intptr_t)0x20D0)
#define llEFCommonEffects2FireSparkMatAnimJoint ((intptr_t)0x2170)
#define llEFCommonEffects2CatchSwirlMObjSub ((intptr_t)0x2760)
#define llEFCommonEffects2CatchSwirlDObjDesc ((intptr_t)0x22B8)
#define llEFCommonEffects2CatchSwirlAnimJoint ((intptr_t)0x28A0)
#define llEFCommonEffects2CatchSwirlMatAnimJoint ((intptr_t)0x2AB0)
#define llEFCommonEffects2ReflectBreakMObjSub ((intptr_t)0x3398)
#define llEFCommonEffects2ReflectBreakDObjDesc ((intptr_t)0x2F78)
#define llEFCommonEffects2ReflectBreakAnimJoint ((intptr_t)0x34A0)
#define llEFCommonEffects2ReflectBreakMatAnimJoint ((intptr_t)0x35A0)
#define llEFCommonEffects2ShadowTextureImage ((intptr_t)0x3A68)
#define llEFCommonEffects2DeadExplodeDefaultMObjSub ((intptr_t)0x53E8)
#define llEFCommonEffects2DeadExplodeDefaultDObjDesc ((intptr_t)0x4F08)
#define llEFCommonEffects2DeadExplodeDefaultAnimJoint ((intptr_t)0x54D0)
#define llEFCommonEffects2DeadExplodeDefaultMatAnimJoint ((intptr_t)0x5870)
#define llEFCommonEffects2DeadExplode1MatAnimJoint ((intptr_t)0x58E0)
#define llEFCommonEffects2DeadExplode2MatAnimJoint ((intptr_t)0x5800)
#define llEFCommonEffects2DeadExplode3MatAnimJoint ((intptr_t)0x5950)
#define llEFCommonEffects2DeadExplode4MatAnimJoint ((intptr_t)0x5870)
#define llEFCommonEffects2NessPKFlashMObjSub ((intptr_t)0x6D00)
#define llEFCommonEffects2NessPKFlashDObjDesc ((intptr_t)0x6B40)
#define llEFCommonEffects2NessPKFlashAnimJoint ((intptr_t)0x6D90)
#define llEFCommonEffects2NessPKFlashMatAnimJoint ((intptr_t)0x6E20)
#define llEFCommonEffects3MBallRaysMObjSub ((intptr_t)0x0108)
#define llEFCommonEffects3MBallRaysDObjDesc ((intptr_t)0x0628)
#define llEFCommonEffects3MBallRaysAnimJoint ((intptr_t)0x0710)
#define llEFCommonEffects3MBallRaysMatAnimJoint ((intptr_t)0x0860)
#define llEFCommonEffects3RebirthHaloDObjDesc ((intptr_t)0x2AC0)
#define llEFCommonEffects3RebirthHaloAnimJoint ((intptr_t)0x2B70)
#define llEFCommonEffects3ItemGetSwirlMObjSub ((intptr_t)0x2CA8)
#define llEFCommonEffects3ItemGetSwirlDObjDesc ((intptr_t)0x3170)
#define llEFCommonEffects3ItemGetSwirlAnimJoint ((intptr_t)0x32B0)
#define llEFCommonEffects3ItemGetSwirlMatAnimJoint ((intptr_t)0x3490)
#define llIFCommonItemArrowSprite ((intptr_t)0x50)
#define llStageDreamLandSprite ((intptr_t)0x26c88)
#define llStageZebesSprite ((intptr_t)0x26c88)
#define llMVOpeningRoomWallpaperSprite ((intptr_t)0x26c88)
#define ll_91_0x26c88Sprite ((intptr_t)0x26c88)
#define llStageJungleSprite ((intptr_t)0x26c88)
#define llStageYoshiSprite ((intptr_t)0x26c88)
#define llStagePokemonSprite ((intptr_t)0x26c88)
#define llStageCastleSprite ((intptr_t)0x26c88)
#define ll_96_0x26c88Sprite ((intptr_t)0x26c88)
#define ll_97_0x26c88Sprite ((intptr_t)0x26c88)
#define ll_98_0x26c88Sprite ((intptr_t)0x26c88)
#define llStageSectorSprite ((intptr_t)0x26c88)
#define ll_119_0x26c88Sprite ((intptr_t)0x26c88)
#define llBonus2CommonPlatformSmallDObjDesc ((intptr_t)0x3DA8)
#define llBonus2CommonPlatformSmallAnimJoint ((intptr_t)0x3E60)
#define llBonus2CommonPlatformSmallMObjSub ((intptr_t)0x3720)
#define llBonus2CommonPlatformSmallMatAnimJoint ((intptr_t)0x3F00)
#define llBonus2CommonPlatformMediumDObjDesc ((intptr_t)0x45D8)
#define llBonus2CommonPlatformMediumAnimJoint ((intptr_t)0x4690)
#define llBonus2CommonPlatformMediumMObjSub ((intptr_t)0x3F70)
#define llBonus2CommonPlatformMediumMatAnimJoint ((intptr_t)0x4730)
#define llBonus2CommonPlatformLargeDObjDesc ((intptr_t)0x4E08)
#define llBonus2CommonPlatformLargeAnimJoint ((intptr_t)0x4EC0)
#define llBonus2CommonPlatformLargeMObjSub ((intptr_t)0x47A0)
#define llBonus2CommonPlatformLargeMatAnimJoint ((intptr_t)0x4F70)
#define llBonus2CommonBoardedPlatformSmallDObjDesc ((intptr_t)0x5520)
#define llBonus2CommonBoardedPlatformSmallAnimJoint ((intptr_t)0x55D0)
#define llBonus2CommonBoardedPlatformMediumDObjDesc ((intptr_t)0x5B80)
#define llBonus2CommonBoardedPlatformMediumAnimJoint ((intptr_t)0x5C30)
#define llBonus2CommonBoardedPlatformLargeDObjDesc ((intptr_t)0x61E0)
#define llBonus2CommonBoardedPlatformLargeAnimJoint ((intptr_t)0x6290)
#define llSC1PStageClear3PlatformSprite ((intptr_t)0xc0)
#define llSC1PStageClear3TargetSprite ((intptr_t)0x1d0)
#define llFoxSpecial3_1B34_AnimJoint ((intptr_t)0x1B34)
#define llFoxSpecial3_1B84_AnimJoint ((intptr_t)0x1B84)
#define llFoxSpecial3EntryArwingDObjDesc ((intptr_t)0x2C30)
#define llFoxSpecial3_2E74_AnimJoint ((intptr_t)0x2E74)
#define llFoxSpecial3_2EB4_AnimJoint ((intptr_t)0x2EB4)
#define llFTManagerCommonShieldDObjDesc ((intptr_t)0x300)
#define llIFCommonPlayerDamageDigit0Sprite ((intptr_t)0x148)
#define llIFCommonPlayerDamageDigit1Sprite ((intptr_t)0x2d8)
#define llIFCommonPlayerDamageDigit2Sprite ((intptr_t)0x500)
#define llIFCommonPlayerDamageDigit3Sprite ((intptr_t)0x698)
#define llIFCommonPlayerDamageDigit4Sprite ((intptr_t)0x8c0)
#define llIFCommonPlayerDamageDigit5Sprite ((intptr_t)0xa58)
#define llIFCommonPlayerDamageDigit6Sprite ((intptr_t)0xc80)
#define llIFCommonPlayerDamageDigit7Sprite ((intptr_t)0xe18)
#define llIFCommonPlayerDamageDigit8Sprite ((intptr_t)0x1040)
#define llIFCommonPlayerDamageDigit9Sprite ((intptr_t)0x1270)
#define llIFCommonPlayerDamageSymbolPercentSprite ((intptr_t)0x1458)
#define llIFCommonPlayerDamageSymbolHPSprite ((intptr_t)0x15d8)
#define llIFCommonTimerDigit0Sprite ((intptr_t)0x138)
#define llIFCommonTimerDigit1Sprite ((intptr_t)0x228)
#define llIFCommonTimerDigit2Sprite ((intptr_t)0x3a8)
#define llIFCommonTimerDigit3Sprite ((intptr_t)0x528)
#define llIFCommonTimerDigit4Sprite ((intptr_t)0x6a8)
#define llIFCommonTimerDigit5Sprite ((intptr_t)0x828)
#define llIFCommonTimerDigit6Sprite ((intptr_t)0x9a8)
#define llIFCommonTimerDigit7Sprite ((intptr_t)0xb28)
#define llIFCommonTimerDigit8Sprite ((intptr_t)0xca8)
#define llIFCommonTimerDigit9Sprite ((intptr_t)0xe28)
#define llIFCommonTimerSymbolColonSprite ((intptr_t)0xf08)
#define llIFCommonTimerSymbolCrossSprite ((intptr_t)0x1018)
#define llIFCommonTimerSymbolUnderscoreSprite ((intptr_t)0x1090)
#define llIFCommonTimerSymbolSecSprite ((intptr_t)0x1140)
#define llIFCommonTimerSymbolCSecSprite ((intptr_t)0x1238)
#define llIFCommonPlayerArrowsDObjDesc ((intptr_t)0x188)
#define llIFCommonPlayerArrowsAnimJoint ((intptr_t)0x270)
#define llIFCommonPlayerMagnifyFrameImage ((intptr_t)0x2C8)
#define llIFCommonPlayerMagnifyDisplayList ((intptr_t)0x030)
#define llMNTitleLogoAnimCutoutSprite ((intptr_t)0x8fc8)
#define llMNTitleLogoAnimStrikeVSprite ((intptr_t)0x97e8)
#define llMNTitleLogoAnimStrikeHSprite ((intptr_t)0x9b48)
#define llMNTitleLogoAnimFullSprite ((intptr_t)0xbbb0)
#define llMNTitleBorderUpperSprite ((intptr_t)0xc208)
#define llMNTitleGradientSprite ((intptr_t)0xf278)
#define llMNTitleTMSprite ((intptr_t)0xf398)
#define llMNTitleCutoutSprite ((intptr_t)0x11988)
#define llMNTitleTMUnkSprite ((intptr_t)0x11aa8)
#define llMNTitleCopyrightSprite ((intptr_t)0x15320)
#define llMNTitlePressStartSprite ((intptr_t)0x15a48)
#define llMNTitleSuperSprite ((intptr_t)0x16728)
#define llMNTitleSmashSprite ((intptr_t)0x245c8)
#define llMNTitleBrosSprite ((intptr_t)0x25188)
#define llMNTitleLogoAnimJoint ((intptr_t)0x251D0)
#define llMNTitleLabelsAnimJoint ((intptr_t)0x25350)
#define llMNTitlePressStartAnimJoint ((intptr_t)0x258D0)
#define llMNTitleSlashAnimJoint ((intptr_t)0x25E70)
#define llMNTitleSlashMatAnimJoint ((intptr_t)0x25F60)
#define llMNTitleLogoDObjDesc ((intptr_t)0x26020)
#define llMNTitleLabelsDObjDesc ((intptr_t)0x26130)
#define llMNTitlePressStartDObjDesc ((intptr_t)0x262C0)
#define llMNTitleSlashMObjSub ((intptr_t)0x287D8)
#define llMNTitleSlashDObjDesc ((intptr_t)0x28DA8)
#define llMNTitleFireDObjDesc ((intptr_t)0x28EB0)
#define llMNTitleFireAnimJoint ((intptr_t)0x29010)
#define llMNTitleFireAnimFrame1Sprite ((intptr_t)0x1018)
#define llMNTitleFireAnimFrame2Sprite ((intptr_t)0x2078)
#define llMNTitleFireAnimFrame3Sprite ((intptr_t)0x30d8)
#define llMNTitleFireAnimFrame4Sprite ((intptr_t)0x4138)
#define llMNTitleFireAnimFrame5Sprite ((intptr_t)0x5198)
#define llMNTitleFireAnimFrame6Sprite ((intptr_t)0x61f8)
#define llMNTitleFireAnimFrame7Sprite ((intptr_t)0x7258)
#define llMNTitleFireAnimFrame8Sprite ((intptr_t)0x82b8)
#define llMNTitleFireAnimFrame9Sprite ((intptr_t)0x9318)
#define llMNTitleFireAnimFrame10Sprite ((intptr_t)0xa378)
#define llMNTitleFireAnimFrame11Sprite ((intptr_t)0xb3d8)
#define llMNTitleFireAnimFrame12Sprite ((intptr_t)0xc438)
#define llMNTitleFireAnimFrame13Sprite ((intptr_t)0xd498)
#define llMNTitleFireAnimFrame14Sprite ((intptr_t)0xe4f8)
#define llMNTitleFireAnimFrame15Sprite ((intptr_t)0xf558)
#define llMNTitleFireAnimFrame16Sprite ((intptr_t)0x105b8)
#define llMNTitleFireAnimFrame17Sprite ((intptr_t)0x11618)
#define llMNTitleFireAnimFrame18Sprite ((intptr_t)0x12678)
#define llMNTitleFireAnimFrame19Sprite ((intptr_t)0x136d8)
#define llMNTitleFireAnimFrame20Sprite ((intptr_t)0x14738)
#define llMNTitleFireAnimFrame21Sprite ((intptr_t)0x15798)
#define llMNTitleFireAnimFrame22Sprite ((intptr_t)0x167f8)
#define llMNTitleFireAnimFrame23Sprite ((intptr_t)0x17858)
#define llMNTitleFireAnimFrame24Sprite ((intptr_t)0x188b8)
#define llMNTitleFireAnimFrame25Sprite ((intptr_t)0x19918)
#define llMNTitleFireAnimFrame26Sprite ((intptr_t)0x1a978)
#define llMNTitleFireAnimFrame27Sprite ((intptr_t)0x1b9d8)
#define llMNTitleFireAnimFrame28Sprite ((intptr_t)0x1ca38)
#define llMNTitleFireAnimFrame29Sprite ((intptr_t)0x1da98)
#define llMNTitleFireAnimFrame30Sprite ((intptr_t)0x1eaf8)
#define llMNNoControllerSprite ((intptr_t)0x8460)
#define llMNCongraKirbyBottomSprite ((intptr_t)0x20718)
#define llMNCongraKirbyTopSprite ((intptr_t)0x20718)
#define llMNCongraYoshiBottomSprite ((intptr_t)0x20718)
#define llMNCongraYoshiTopSprite ((intptr_t)0x20718)
#define llMNCongraPikachuBottomSprite ((intptr_t)0x20718)
#define llMNCongraPikachuTopSprite ((intptr_t)0x20718)
#define llMNCongraSamusBottomSprite ((intptr_t)0x20718)
#define llMNCongraSamusTopSprite ((intptr_t)0x20718)
#define llMNCongraLinkBottomSprite ((intptr_t)0x20718)
#define llMNCongraLinkTopSprite ((intptr_t)0x20718)
#define llMNCongraPurinBottomSprite ((intptr_t)0x20718)
#define llMNCongraPurinTopSprite ((intptr_t)0x20718)
#define llMNCongraCaptainBottomSprite ((intptr_t)0x20718)
#define llMNCongraCaptainTopSprite ((intptr_t)0x20718)
#define llMNCongraDonkeyBottomSprite ((intptr_t)0x20718)
#define llMNCongraDonkeyTopSprite ((intptr_t)0x20718)
#define llMNCongraMarioBottomSprite ((intptr_t)0x20718)
#define llMNCongraMarioTopSprite ((intptr_t)0x20718)
#define llMNCongraLuigiBottomSprite ((intptr_t)0x20718)
#define llMNCongraLuigiTopSprite ((intptr_t)0x20718)
#define llMNCongraFoxBottomSprite ((intptr_t)0x20718)
#define llMNCongraFoxTopSprite ((intptr_t)0x20718)
#define llMNCongraNessBottomSprite ((intptr_t)0x20718)
#define llMNCongraNessTopSprite ((intptr_t)0x20718)
#define llN64LogoSprite ((intptr_t)0x73c0)
#define llSCStaffrollNameAndJobAUpperImage ((intptr_t)0x0008)
#define llSCStaffrollNameAndJobALowerImage ((intptr_t)0x0178)
#define llSCStaffrollNameAndJobBUpperImage ((intptr_t)0x0218)
#define llSCStaffrollNameAndJobBLowerImage ((intptr_t)0x02D8)
#define llSCStaffrollNameAndJobCUpperImage ((intptr_t)0x0398)
#define llSCStaffrollNameAndJobCLowerImage ((intptr_t)0x0458)
#define llSCStaffrollNameAndJobDUpperImage ((intptr_t)0x04F8)
#define llSCStaffrollNameAndJobDLowerImage ((intptr_t)0x0668)
#define llSCStaffrollNameAndJobEUpperImage ((intptr_t)0x0728)
#define llSCStaffrollNameAndJobELowerImage ((intptr_t)0x07E8)
#define llSCStaffrollNameAndJobFUpperImage ((intptr_t)0x0888)
#define llSCStaffrollNameAndJobFLowerImage ((intptr_t)0x0948)
#define llSCStaffrollNameAndJobGUpperImage ((intptr_t)0x0A08)
#define llSCStaffrollNameAndJobGLowerImage ((intptr_t)0x0B78)
#define llSCStaffrollNameAndJobHUpperImage ((intptr_t)0x0C38)
#define llSCStaffrollNameAndJobHLowerImage ((intptr_t)0x0DA8)
#define llSCStaffrollNameAndJobIUpperImage ((intptr_t)0x0E68)
#define llSCStaffrollNameAndJobILowerImage ((intptr_t)0x0F28)
#define llSCStaffrollNameAndJobJUpperImage ((intptr_t)0x0FE8)
#define llSCStaffrollNameAndJobJLowerImage ((intptr_t)0x10A8)
#define llSCStaffrollNameAndJobKUpperImage ((intptr_t)0x1188)
#define llSCStaffrollNameAndJobKLowerImage ((intptr_t)0x12F8)
#define llSCStaffrollNameAndJobLUpperImage ((intptr_t)0x13B8)
#define llSCStaffrollNameAndJobLLowerImage ((intptr_t)0x1478)
#define llSCStaffrollNameAndJobMUpperImage ((intptr_t)0x1538)
#define llSCStaffrollNameAndJobMLowerImage ((intptr_t)0x16A8)
#define llSCStaffrollNameAndJobNUpperImage ((intptr_t)0x17D8)
#define llSCStaffrollNameAndJobNLowerImage ((intptr_t)0x1948)
#define llSCStaffrollNameAndJobOUpperImage ((intptr_t)0x19E8)
#define llSCStaffrollNameAndJobOLowerImage ((intptr_t)0x1B58)
#define llSCStaffrollNameAndJobPUpperImage ((intptr_t)0x1C88)
#define llSCStaffrollNameAndJobPLowerImage ((intptr_t)0x1D48)
#define llSCStaffrollNameAndJobQUpperImage ((intptr_t)0x1E08)
#define llSCStaffrollNameAndJobQLowerImage ((intptr_t)0x1F78)
#define llSCStaffrollNameAndJobRUpperImage ((intptr_t)0x2038)
#define llSCStaffrollNameAndJobRLowerImage ((intptr_t)0x20F8)
#define llSCStaffrollNameAndJobSUpperImage ((intptr_t)0x2198)
#define llSCStaffrollNameAndJobSLowerImage ((intptr_t)0x2258)
#define llSCStaffrollNameAndJobTUpperImage ((intptr_t)0x22F8)
#define llSCStaffrollNameAndJobTLowerImage ((intptr_t)0x23B8)
#define llSCStaffrollNameAndJobUUpperImage ((intptr_t)0x2478)
#define llSCStaffrollNameAndJobULowerImage ((intptr_t)0x2538)
#define llSCStaffrollNameAndJobVUpperImage ((intptr_t)0x25D8)
#define llSCStaffrollNameAndJobVLowerImage ((intptr_t)0x2748)
#define llSCStaffrollNameAndJobWUpperImage ((intptr_t)0x27E8)
#define llSCStaffrollNameAndJobWLowerImage ((intptr_t)0x2958)
#define llSCStaffrollNameAndJobXUpperImage ((intptr_t)0x2A88)
#define llSCStaffrollNameAndJobXLowerImage ((intptr_t)0x2BF8)
#define llSCStaffrollNameAndJobYUpperImage ((intptr_t)0x2C98)
#define llSCStaffrollNameAndJobYLowerImage ((intptr_t)0x2D58)
#define llSCStaffrollNameAndJobZUpperImage ((intptr_t)0x2E18)
#define llSCStaffrollNameAndJobZLowerImage ((intptr_t)0x2F88)
#define llSCStaffrollNameAndJobCommaImage ((intptr_t)0x3018)
#define llSCStaffrollNameAndJobPeriodImage ((intptr_t)0x3078)
#define llSCStaffrollNameAndJobApostropheImage ((intptr_t)0x30B8)
#define llSCStaffrollNameAndJob4Image ((intptr_t)0x3118)
#define llSCStaffrollTextBoxAUpperSprite ((intptr_t)0x3258)
#define llSCStaffrollTextBoxALowerSprite ((intptr_t)0x3310)
#define llSCStaffrollTextBoxBUpperSprite ((intptr_t)0x33e8)
#define llSCStaffrollTextBoxBLowerSprite ((intptr_t)0x34b0)
#define llSCStaffrollTextBoxCUpperSprite ((intptr_t)0x3588)
#define llSCStaffrollTextBoxCLowerSprite ((intptr_t)0x3640)
#define llSCStaffrollTextBoxDUpperSprite ((intptr_t)0x3718)
#define llSCStaffrollTextBoxDLowerSprite ((intptr_t)0x37e0)
#define llSCStaffrollTextBoxEUpperSprite ((intptr_t)0x38b8)
#define llSCStaffrollTextBoxELowerSprite ((intptr_t)0x3970)
#define llSCStaffrollTextBoxFUpperSprite ((intptr_t)0x3a48)
#define llSCStaffrollTextBoxFLowerSprite ((intptr_t)0x3b10)
#define llSCStaffrollTextBoxGUpperSprite ((intptr_t)0x3be8)
#define llSCStaffrollTextBoxGLowerSprite ((intptr_t)0x3ca8)
#define llSCStaffrollTextBoxHUpperSprite ((intptr_t)0x3d78)
#define llSCStaffrollTextBoxHLowerSprite ((intptr_t)0x3e40)
#define llSCStaffrollTextBoxIUpperSprite ((intptr_t)0x3f18)
#define llSCStaffrollTextBoxILowerSprite ((intptr_t)0x3fe0)
#define llSCStaffrollTextBoxJUpperSprite ((intptr_t)0x40b8)
#define llSCStaffrollTextBoxJLowerSprite ((intptr_t)0x4188)
#define llSCStaffrollTextBoxKUpperSprite ((intptr_t)0x4258)
#define llSCStaffrollTextBoxKLowerSprite ((intptr_t)0x4320)
#define llSCStaffrollTextBoxLUpperSprite ((intptr_t)0x43f8)
#define llSCStaffrollTextBoxLLowerSprite ((intptr_t)0x44c0)
#define llSCStaffrollTextBoxMUpperSprite ((intptr_t)0x4598)
#define llSCStaffrollTextBoxMLowerSprite ((intptr_t)0x4650)
#define llSCStaffrollTextBoxNUpperSprite ((intptr_t)0x4728)
#define llSCStaffrollTextBoxNLowerSprite ((intptr_t)0x47e0)
#define llSCStaffrollTextBoxOUpperSprite ((intptr_t)0x48b8)
#define llSCStaffrollTextBoxOLowerSprite ((intptr_t)0x4970)
#define llSCStaffrollTextBoxPUpperSprite ((intptr_t)0x4a48)
#define llSCStaffrollTextBoxPLowerSprite ((intptr_t)0x4b08)
#define llSCStaffrollTextBoxQUpperSprite ((intptr_t)0x4bd8)
#define llSCStaffrollTextBoxQLowerSprite ((intptr_t)0x4c98)
#define llSCStaffrollTextBoxRUpperSprite ((intptr_t)0x4d68)
#define llSCStaffrollTextBoxRLowerSprite ((intptr_t)0x4e20)
#define llSCStaffrollTextBoxSUpperSprite ((intptr_t)0x4ef8)
#define llSCStaffrollTextBoxSLowerSprite ((intptr_t)0x4fb0)
#define llSCStaffrollTextBoxTUpperSprite ((intptr_t)0x5088)
#define llSCStaffrollTextBoxTLowerSprite ((intptr_t)0x5150)
#define llSCStaffrollTextBoxUUpperSprite ((intptr_t)0x5228)
#define llSCStaffrollTextBoxULowerSprite ((intptr_t)0x52e0)
#define llSCStaffrollTextBoxVUpperSprite ((intptr_t)0x53b8)
#define llSCStaffrollTextBoxVLowerSprite ((intptr_t)0x5470)
#define llSCStaffrollTextBoxWUpperSprite ((intptr_t)0x5548)
#define llSCStaffrollTextBoxWLowerSprite ((intptr_t)0x5600)
#define llSCStaffrollTextBoxXUpperSprite ((intptr_t)0x56d8)
#define llSCStaffrollTextBoxXLowerSprite ((intptr_t)0x5790)
#define llSCStaffrollTextBoxYUpperSprite ((intptr_t)0x5868)
#define llSCStaffrollTextBoxYLowerSprite ((intptr_t)0x5928)
#define llSCStaffrollTextBoxZUpperSprite ((intptr_t)0x59f8)
#define llSCStaffrollTextBoxZLowerSprite ((intptr_t)0x5ab0)
#define llSCStaffrollTextBoxColonSprite ((intptr_t)0x5b70)
#define llSCStaffrollTextBoxCommaSprite ((intptr_t)0x5c00)
#define llSCStaffrollTextBoxPeriodSprite ((intptr_t)0x5c90)
#define llSCStaffrollTextBoxDashSprite ((intptr_t)0x5d18)
#define llSCStaffrollTextBox1Sprite ((intptr_t)0x5de8)
#define llSCStaffrollTextBox2Sprite ((intptr_t)0x5eb8)
#define llSCStaffrollTextBox3Sprite ((intptr_t)0x5f88)
#define llSCStaffrollTextBox4Sprite ((intptr_t)0x6058)
#define llSCStaffrollTextBox5Sprite ((intptr_t)0x6128)
#define llSCStaffrollTextBox6Sprite ((intptr_t)0x61f8)
#define llSCStaffrollTextBox7Sprite ((intptr_t)0x62c8)
#define llSCStaffrollTextBox8Sprite ((intptr_t)0x6398)
#define llSCStaffrollTextBox9Sprite ((intptr_t)0x6468)
#define llSCStaffrollTextBox0Sprite ((intptr_t)0x6538)
#define llSCStaffrollTextBoxQuoteSprite ((intptr_t)0x65c0)
#define llSCStaffrollTextBoxAmpersSprite ((intptr_t)0x6698)
#define llSCStaffrollTextBoxSlashSprite ((intptr_t)0x6758)
#define llSCStaffrollTextBoxApostropheSprite ((intptr_t)0x67e0)
#define llSCStaffrollTextBoxQuestionSprite ((intptr_t)0x68b8)
#define llSCStaffrollTextBoxBracketOpenSprite ((intptr_t)0x6988)
#define llSCStaffrollTextBoxBracketCloseSprite ((intptr_t)0x6a58)
#define llSCStaffrollTextBoxEAccentSprite ((intptr_t)0x6b20)
#define llSCStaffrollCrosshairSprite ((intptr_t)0x6D58)
#define llSCStaffrollTextBoxBracketLeftSprite ((intptr_t)0x6F98)
#define llSCStaffrollInterpolation ((intptr_t)0x7304)
#define llSCStaffrollAnimJoint ((intptr_t)0x7338)
#define llSCStaffrollDObjDesc ((intptr_t)0x78C0)
#define llSCStaffrollTextBoxBracketRightSprite ((intptr_t)0x71D8)
#define llMNSoundTestMusicTextSprite ((intptr_t)0x438)
#define llMNSoundTestSoundTextSprite ((intptr_t)0x9c0)
#define llMNSoundTestVoiceTextSprite ((intptr_t)0xe48)
#define llMNSoundTestCapsuleRightSprite ((intptr_t)0x1138)
#define llMNSoundTestColonExitTextSprite ((intptr_t)0x1208)
#define llMNSoundTestColonFadeOutTextSprite ((intptr_t)0x1348)
#define llMNSoundTestColonPlayTextSprite ((intptr_t)0x1450)
#define llMNSoundTestSoundTestTextSprite ((intptr_t)0x1bb8)
#define llMNSoundTestStartButtonSprite ((intptr_t)0x1d50)
#define llIFCommonBattlePausePlayerNum1PSprite ((intptr_t)0x78)
#define llIFCommonBattlePausePlayerNum2PSprite ((intptr_t)0x138)
#define llIFCommonBattlePausePlayerNum3PSprite ((intptr_t)0x1f8)
#define llIFCommonBattlePausePlayerNum4PSprite ((intptr_t)0x2b8)
#define llIFCommonBattlePauseDecalPauseSprite ((intptr_t)0x438)
#define llIFCommonBattlePauseDecalPlusSprite ((intptr_t)0x4d8)
#define llIFCommonBattlePauseDecalResetSprite ((intptr_t)0x610)
#define llIFCommonBattlePauseDecalSmashBallSprite ((intptr_t)0x6d8)
#define llIFCommonBattlePauseDecalRetrySprite ((intptr_t)0x828)
#define llIFCommonBattlePauseDecalAButtonSprite ((intptr_t)0x958)
#define llIFCommonBattlePauseDecalBButtonSprite ((intptr_t)0xa88)
#define llIFCommonBattlePauseDecalZTriggerSprite ((intptr_t)0xbd8)
#define llIFCommonBattlePauseDecalRTriggerSprite ((intptr_t)0xcf8)
#define llIFCommonBattlePauseDecalArrowsSprite ((intptr_t)0x1538)
#define llIFCommonBattlePauseDecalControlStickSprite ((intptr_t)0x17a8)
#define llIFCommonBattlePauseDecalLTriggerSprite ((intptr_t)0x18c8)
#define llSCExplainGraphicsStickMObjSub ((intptr_t)0x5028)
#define llSCExplainGraphicsStickDObjDesc ((intptr_t)0x5300)
#define llSCExplainGraphicsStickNeutralMatAnimJoint ((intptr_t)0x5390)
#define llSCExplainGraphicsStickHoldUpMatAnimJoint ((intptr_t)0x53C0)
#define llSCExplainGraphicsStickTapUpMatAnimJoint ((intptr_t)0x53F0)
#define llSCExplainGraphicsStickHoldForwardMatAnimJoint ((intptr_t)0x5430)
#define llSCExplainGraphicsStickTapForwardMatAnimJoint ((intptr_t)0x5450)
#define llSCExplainGraphicsTapSparkMObjSub ((intptr_t)0x5A98)
#define llSCExplainGraphicsTapSparkDisplayList ((intptr_t)0x5B68)
#define llSCExplainGraphicsTapSparkMatAnimJoint ((intptr_t)0x5C20)
#define llSCExplainGraphicsSpecialMoveRGBDisplayList ((intptr_t)0x5E40)
#define llSCExplainGraphicsDamage1Sprite ((intptr_t)0x6c58)
#define llSCExplainGraphicsDamage2Sprite ((intptr_t)0x72d8)
#define llSCExplainGraphicsDamage3Sprite ((intptr_t)0x7c38)
#define llSCExplainGraphicsShieldSprite ((intptr_t)0x8218)
#define llSCExplainGraphicsStartFightingSprite ((intptr_t)0x8c78)
#define llSCExplainGraphicsPlayerCountSprite ((intptr_t)0x91a8)
#define llSCExplainGraphicsHereTextSprite ((intptr_t)0x9628)
#define llSCExplainGraphicsBannerSprite ((intptr_t)0x10260)
#define llSCExplainGraphicsTapTheStickSprite ((intptr_t)0x11f60)
#define llSCExplainGraphicsJumpSprite ((intptr_t)0x12b60)
#define llSCExplainGraphicsMidairJumpSprite ((intptr_t)0x13658)
#define llSCExplainGraphicsAttackSprite ((intptr_t)0x139f0)
#define llSCExplainGraphicsPowerAttackSprite ((intptr_t)0x14448)
#define llSCExplainGraphicsSmashAttackSprite ((intptr_t)0x14e30)
#define llSCExplainGraphicsKnockThemOffSprite ((intptr_t)0x15c40)
#define llSCExplainGraphicsBUpGetBackSprite ((intptr_t)0x17fe0)
#define llSCExplainGraphicsSpecialMovesSprite ((intptr_t)0x1a440)
#define llSCExplainGraphicsDodgeSprite ((intptr_t)0x1aa10)
#define llSCExplainGraphicsThrowEnemySprite ((intptr_t)0x1b468)
#define llSCExplainGraphicsGrabItemsSprite ((intptr_t)0x1b950)
#define llSCExplainGraphicsUseItemsSprite ((intptr_t)0x1beb0)
#define llSCExplainGraphicsThrowItemsSprite ((intptr_t)0x1cd20)
#define llSCExplainGraphicsAButtonSprite ((intptr_t)0x1d338)
#define llSCExplainGraphicsBButtonSprite ((intptr_t)0x1d948)
#define llSCExplainGraphicsZButtonSprite ((intptr_t)0x1df58)
#define llSCExplainGraphicsPlusSymbolSprite ((intptr_t)0x1e018)
#define llSYKseg1ValidateFunc ((intptr_t)0x0)
#define llSYKseg1ValidateNBytes ((intptr_t)0x30)
#define llSYSignValidateFunc ((intptr_t)0x0)
#define llSYSignValidateNBytes ((intptr_t)0x40)
#define llMarioMainFireballWeaponAttributes ((intptr_t)0x0)
#define llMarioSpecial1FireballWeaponAttributes ((intptr_t)0x0)
#define llFoxMainMotionLwReflectorFTSpecialColl ((intptr_t)0x19B0)
#define llFoxSpecial1BlasterWeaponAttributes ((intptr_t)0x0)
#define llSamusMainBombWeaponAttributes ((intptr_t)0xC)
#define llSamusSpecial1ChargeShotWeaponAttributes ((intptr_t)0x0)
#define llLuigiSpecial1FireballWeaponAttributes ((intptr_t)0x0)
#define llLinkMainSpinAttackWeaponAttributes ((intptr_t)0xC)
#define llLinkMainBombItemAttributes ((intptr_t)0x40)
#define llLinkMainBombAttackEvents ((intptr_t)0x88)
#define llLinkMainBombBloatScales ((intptr_t)0xA8)
#define llLinkSpecial1BoomerangWeaponAttributes ((intptr_t)0x0)
#define llKirbyMainMotionSpecialNFTKirbyCopy ((intptr_t)0x0)
#define llKirbyMainMotionftKirbyAttack100Effect ((intptr_t)0x1220)
#define llKirbyMainCutterWeaponAttributes ((intptr_t)0x8)
#define llCaptainMainMotionSpecialHiVec2h ((intptr_t)0x0)
#define llNessMainMotionAttackS4ReflectorFTSpecialColl ((intptr_t)0x1114)
#define llNessMainMotionLwAbsorbFTSpecialColl ((intptr_t)0x16D4)
#define llNessMainPKThunderWeaponAttributes ((intptr_t)0xC)
#define llNessMainPKThunderTrailWeaponAttributes ((intptr_t)0x40)
#define llNessSpecial1PKFireWeaponAttributes ((intptr_t)0x0)
#define llNessSpecial1PKFireItemAttributes ((intptr_t)0x34)
#define llPikachuMainThunderHeadWeaponAttributes ((intptr_t)0xC)
#define llPikachuMainThunderTrailWeaponAttributes ((intptr_t)0x40)
#define llPikachuSpecial1ThunderJoltAirWeaponAttributes ((intptr_t)0x0)
#define llPikachuSpecial1ThunderJoltGroundWeaponAttributes ((intptr_t)0x34)
#define llYoshiMainEggThrowWeaponAttributes ((intptr_t)0xC)
#define llYoshiMainStarWeaponAttributes ((intptr_t)0x40)
#define llBossMainMotionBulletNormalWeaponAttributes ((intptr_t)0x774)
#define llBossMainMotionBulletHardWeaponAttributes ((intptr_t)0x7A8)
#define llITCommonDataContainerVelocitiesY ((intptr_t)0x0)
#define llITCommonDataCapsuleItemAttributes ((intptr_t)0x50)
#define llITCommonDataCapsuleAttackEvents ((intptr_t)0x98)
#define llITCommonDataTomatoItemAttributes ((intptr_t)0xB8)
#define llITCommonDataHeartItemAttributes ((intptr_t)0x100)
#define llITCommonDataStarItemAttributes ((intptr_t)0x148)
#define llITCommonDataSwordItemAttributes ((intptr_t)0x190)
#define llITCommonDataBatItemAttributes ((intptr_t)0x1D8)
#define llITCommonDataHarisenItemAttributes ((intptr_t)0x220)
#define llITCommonDataLGunItemAttributes ((intptr_t)0x268)
#define llITCommonDataLGunAmmoWeaponAttributes ((intptr_t)0x2B0)
#define llITCommonDataFFlowerItemAttributes ((intptr_t)0x2E4)
#define llITCommonDataFFlowerFlameWeaponAttributes ((intptr_t)0x32C)
#define llITCommonDataFFlowerFlameAngles ((intptr_t)0x360)
#define llITCommonDataHammerItemAttributes ((intptr_t)0x374)
#define llITCommonDataMSBombItemAttributes ((intptr_t)0x3BC)
#define llITCommonDataMSBombAttackEvents ((intptr_t)0x404)
#define llITCommonDataBombHeiItemAttributes ((intptr_t)0x424)
#define llITCommonDataBombHeiAttackEvents ((intptr_t)0x46C)
#define llITCommonDataStarRodItemAttributes ((intptr_t)0x48C)
#define llITCommonDataStarRodWeaponAttributes ((intptr_t)0x4D4)
#define llITCommonDataStarRodSmashWeaponAttributes ((intptr_t)0x508)
#define llITCommonDataGShellItemAttributes ((intptr_t)0x53C)
#define llITCommonDataRShellItemAttributes ((intptr_t)0x584)
#define llITCommonDataBoxItemAttributes ((intptr_t)0x5CC)
#define llITCommonDataBoxAttackEvents ((intptr_t)0x614)
#define llITCommonDataTaruItemAttributes ((intptr_t)0x634)
#define llITCommonDataTaruAttackEvents ((intptr_t)0x67C)
#define llITCommonDataMBallThrownFileHead ((intptr_t)0x6E4)
#define llITCommonDataMBallItemAttributes ((intptr_t)0x6E4)
#define llITCommonDataWarkItemAttributes ((intptr_t)0x72C)
#define llITCommonDataWarkRockWeaponAttributes ((intptr_t)0x774)
#define llITCommonDataKabigonItemAttributes ((intptr_t)0x7A8)
#define llITCommonDataTosakintoItemAttributes ((intptr_t)0x7F0)
#define llITCommonDataMewItemAttributes ((intptr_t)0x838)
#define llITCommonDataNyarsItemAttributes ((intptr_t)0x880)
#define llITCommonDataNyarsCoinWeaponAttributes ((intptr_t)0x8C8)
#define llITCommonDataLizardonItemAttributes ((intptr_t)0x8FC)
#define llITCommonDataLizardonFlameWeaponAttributes ((intptr_t)0x944)
#define llITCommonDataSpearItemAttributes ((intptr_t)0x98C)
#define llITCommonDataSpearSwarmWeaponAttributes ((intptr_t)0x9D4)
#define llITCommonDataKamexItemAttributes ((intptr_t)0xA08)
#define llITCommonDataKamexHydroWeaponAttributes ((intptr_t)0xA50)
#define llITCommonDataMLuckyItemAttributes ((intptr_t)0xA84)
#define llITCommonDataEggItemAttributes ((intptr_t)0xACC)
#define llITCommonDataEggAttackEvents ((intptr_t)0xB14)
#define llITCommonDataStarmieItemAttributes ((intptr_t)0xB34)
#define llITCommonDataStarmieSwiftWeaponAttributes ((intptr_t)0xB7C)
#define llITCommonDataSawamuraItemAttributes ((intptr_t)0xBB0)
#define llITCommonDataDogasItemAttributes ((intptr_t)0xBF8)
#define llITCommonDataDogasSmogWeaponAttributes ((intptr_t)0xC40)
#define llITCommonDataPippiItemAttributes ((intptr_t)0xC74)
#define llITCommonDataPippiSwarmWeaponAttributes ((intptr_t)0xCBC)
#define llITCommonDataGBumperItemAttributes ((intptr_t)0xCF0)
#define llITCommonDataHarisenDataStart ((intptr_t)0x2198)
#define llITCommonDataBombHeiDataStart ((intptr_t)0x33F8)
#define llITCommonDataBombHeiWalkRightDisplayList ((intptr_t)0x3310)
#define llITCommonDataBombHeiWalkLeftDisplayList ((intptr_t)0x34C0)
#define llITCommonDataBombHeiWalkMatAnimJoint ((intptr_t)0x35B8)
#define llITCommonDataKirbyStarDObjDesc ((intptr_t)0x5458)
#define llITCommonDataShellDataStart ((intptr_t)0x5F88)
#define llITCommonDataShellAnimJoint ((intptr_t)0x6018)
#define llITCommonDataShellMatAnimJoint ((intptr_t)0x6048)
#define llITCommonDataBoxDataStart ((intptr_t)0x6778)
#define llITCommonDataBoxEffectDisplayList ((intptr_t)0x68F0)
#define llITCommonDataNBumperItemAttributes ((intptr_t)0x69C)
#define llITCommonDataNBumperDataStart ((intptr_t)0x7648)
#define llITCommonDataNBumperWaitMObjSub ((intptr_t)0x7A38)
#define llITCommonDataNBumperWaitDisplayList ((intptr_t)0x7AF8)
#define llITCommonDataMBallThrownMObjSub ((intptr_t)0x9120)
#define llITCommonDataMBallDataStart ((intptr_t)0x9430)
#define llITCommonDataMBallThrownDObjDesc ((intptr_t)0x9430)
#define llITCommonDataMBallMatAnimJoint ((intptr_t)0x9520)
#define llITCommonDataMBallThrownLAnimJoint ((intptr_t)0x95E0)
#define llITCommonDataMBallThrownRAnimJoint ((intptr_t)0x9690)
#define llITCommonDataMBallThrownLMatAnimJoint ((intptr_t)0x9740)
#define llITCommonDataMBallThrownRMatAnimJoint ((intptr_t)0x9810)
#define llITCommonDataWarkDataStart ((intptr_t)0xA140)
#define llITCommonDataWarkDisplayList ((intptr_t)0xA640)
#define llITCommonDataKabigonAnimJoint ((intptr_t)0xB158)
#define llITCommonDataTosakintoDataStart ((intptr_t)0xB708)
#define llITCommonDataTosakintoAnimJoint ((intptr_t)0xB7CC)
#define llITCommonDataTosakintoMatAnimJoint ((intptr_t)0xB90C)
#define llITCommonDataMewDataStart ((intptr_t)0xBCC0)
#define llITCommonDataNyarsAnimJoint ((intptr_t)0xC130)
#define llITCommonDataLizardonDataStart ((intptr_t)0xD5C0)
#define llITCommonDataLizardonAnimJoint ((intptr_t)0xD658)
#define llITCommonDataLizardonMatAnimJoint ((intptr_t)0xD688)
#define llITCommonDataSpearDataStart ((intptr_t)0xDF38)
#define llITCommonDataSpearAnimJoint ((intptr_t)0xDFFC)
#define llITCommonDataSpearMatAnimJoint ((intptr_t)0xE12C)
#define llITCommonDataKamexDisplayList ((intptr_t)0xED60)
#define llITCommonDataKamexDataStart ((intptr_t)0xEA60)
#define llITCommonDataStarmieDataStart ((intptr_t)0x112A0)
#define llITCommonDataStarmieMatAnimJoint ((intptr_t)0x11338)
#define llITCommonDataDogasDataStart ((intptr_t)0x12820)
#define llITCommonDataDogasAnimJoint ((intptr_t)0x128DC)
#define llITCommonDataLuckyDataStart ((intptr_t)0x10000)
#define llITCommonDataLuckyAnimJoint ((intptr_t)0x100BC)
#define llITCommonDataSawamuraDataStart ((intptr_t)0x11F40)
#define llITCommonDataSawamuraDisplayList ((intptr_t)0x12340)
#define llITCommonDataPippiDataStart ((intptr_t)0x13598)
#define llITCommonDataMonsterAnimBankStart ((intptr_t)0x13624)
#define llSCExplainMain0KeyEvent ((intptr_t)0x0)
#define llSCExplainMain1KeyEvent ((intptr_t)0x9D4)
#define llSCExplainMain2KeyEvent ((intptr_t)0x13FC)
#define llSCExplainMain3KeyEvent ((intptr_t)0x1400)
#define llSCExplainMainExplainPhase ((intptr_t)0x1404)
#define llSC1PTrainingModeDisplayLabelPosSpriteArray ((intptr_t)0x000)
#define llSC1PTrainingModeDisplayOptionSpriteArray ((intptr_t)0x020)
#define llSC1PTrainingModeMenuLabelPosSpriteArray ((intptr_t)0x0BC)
#define llSC1PTrainingModeMenuOptionSpriteArray ((intptr_t)0x13C)
#define llSC1PTrainingMode0x10CPosSpriteArray ((intptr_t)0x10C)
#define llSC1PTrainingMode0x1B8PosSpriteArray ((intptr_t)0x1B8)
#define llGRPupupuMapMapHeader ((intptr_t)0x14)
#define llGRPupupuMapWhispyEyesTransformKindsMObjSub ((intptr_t)0x0F00)
#define llGRPupupuMapMapHead ((intptr_t)0x10F0)
#define llGRPupupuMapWhispyEyesTransformKindsDObjDesc ((intptr_t)0x10F0)
#define llGRPupupuMapWhispyMouthTransformKindsMObjSub ((intptr_t)0x13B0)
#define llGRPupupuMapWhispyMouthTransformKindsDObjDesc ((intptr_t)0x1770)
#define llGRPupupuMapFlowersBackTransformKindsDObjDesc ((intptr_t)0x2A80)
#define llGRPupupuMapFlowersFrontTransformKindsDObjDesc ((intptr_t)0x31F8)
#define llGRPupupuMapWhispyEyesLeftTurnAnimJoint ((intptr_t)0x11A0)
#define llGRPupupuMapWhispyEyesLeftTurnMatAnimJoint ((intptr_t)0x11E0)
#define llGRPupupuMapWhispyEyesLeftBlinkAnimJoint ((intptr_t)0x12B0)
#define llGRPupupuMapWhispyEyesRightTurnAnimJoint ((intptr_t)0x1220)
#define llGRPupupuMapWhispyEyesRightTurnMatAnimJoint ((intptr_t)0x1270)
#define llGRPupupuMapWhispyEyesRightBlinkAnimJoint ((intptr_t)0x1330)
#define llGRPupupuMapWhispyMouthLeftStretchAnimJoint ((intptr_t)0x18B0)
#define llGRPupupuMapWhispyMouthLeftStretchMatAnimJoint ((intptr_t)0x1A00)
#define llGRPupupuMapWhispyMouthLeftTurnAnimJoint ((intptr_t)0x1BE0)
#define llGRPupupuMapWhispyMouthLeftTurnMatAnimJoint ((intptr_t)0x1CE0)
#define llGRPupupuMapWhispyMouthLeftOpenAnimJoint ((intptr_t)0x1E80)
#define llGRPupupuMapWhispyMouthLeftOpenMatAnimJoint ((intptr_t)0x20B0)
#define llGRPupupuMapWhispyMouthLeftCloseAnimJoint ((intptr_t)0x2100)
#define llGRPupupuMapWhispyMouthLeftCloseMatAnimJoint ((intptr_t)0x22A0)
#define llGRPupupuMapWhispyMouthRightStretchAnimJoint ((intptr_t)0x1A40)
#define llGRPupupuMapWhispyMouthRightStretchMatAnimJoint ((intptr_t)0x1BA0)
#define llGRPupupuMapWhispyMouthRightTurnAnimJoint ((intptr_t)0x1D30)
#define llGRPupupuMapWhispyMouthRightTurnMatAnimJoint ((intptr_t)0x1E30)
#define llGRPupupuMapWhispyMouthRightOpenAnimJoint ((intptr_t)0x22F0)
#define llGRPupupuMapWhispyMouthRightOpenMatAnimJoint ((intptr_t)0x2540)
#define llGRPupupuMapWhispyMouthRightCloseAnimJoint ((intptr_t)0x2590)
#define llGRPupupuMapWhispyMouthRightCloseMatAnimJoint ((intptr_t)0x2740)
#define llGRPupupuMapWhispyMouthLeftOpenTexture ((intptr_t)0x2BE0)
#define llGRPupupuMapWhispyMouthLeftBlowTexture ((intptr_t)0x2C30)
#define llGRPupupuMapWhispyMouthLeftCloseTexture ((intptr_t)0x2C80)
#define llGRPupupuMapWhispyMouthRightOpenTexture ((intptr_t)0x2CD0)
#define llGRPupupuMapWhispyMouthRightBlowTexture ((intptr_t)0x2D20)
#define llGRPupupuMapWhispyMouthRightCloseTexture ((intptr_t)0x2D70)
#define llGRPupupuMapWhispyEyesLeft0Texture ((intptr_t)0x33E0)
#define llGRPupupuMapWhispyEyesLeft1Texture ((intptr_t)0x3450)
#define llGRPupupuMapWhispyEyesLeft2Texture ((intptr_t)0x34B0)
#define llGRPupupuMapWhispyEyesRight0Texture ((intptr_t)0x3510)
#define llGRPupupuMapWhispyEyesRight1Texture ((intptr_t)0x35C0)
#define llGRPupupuMapWhispyEyesRight2Texture ((intptr_t)0x3660)
#define llGRPupupuMapBrontoDObjDesc ((intptr_t)0x33B8)
#define llGRPupupuMapBrontoLAnimJoint ((intptr_t)0x3440)
#define llGRPupupuMapBrontoLMatAnimJoint ((intptr_t)0x34E0)
#define llGRPupupuMapBrontoMObjSub ((intptr_t)0x31F0)
#define llGRPupupuMapBrontoRAnimJoint ((intptr_t)0x36E0)
#define llGRPupupuMapBrontoRMatAnimJoint ((intptr_t)0x37A0)
#define llGRPupupuMapDataStart ((intptr_t)0x1CE0)
#define llGRPupupuMapDededeDObjDesc ((intptr_t)0x3E80)
#define llGRPupupuMapDededeFarAnimJoint ((intptr_t)0x3F10)
#define llGRPupupuMapDededeNearAnimJoint ((intptr_t)0x4080)
#define llGRPupupuSmallMapMapHeader ((intptr_t)0x14)
#define llGRZebesMapMapHeader ((intptr_t)0x14)
#define llGRZebesMapAcidGRAttackColl ((intptr_t)0xBC)
#define llGRZebesMapAcidMObjSub ((intptr_t)0x8C0)
#define llGRZebesMapAcidDObjDesc ((intptr_t)0xB08)
#define llGRZebesMapAcidAnimJoint ((intptr_t)0xB90)
#define llGRZebesMapAcidMatAnimJoint ((intptr_t)0xBD0)
#define llGRZebesMapDataStart ((intptr_t)0x5AC0)
#define llGRZebesMapRidleyAnimJoint ((intptr_t)0xCF70)
#define llGRZebesMapRidleyDObjDesc ((intptr_t)0xCEE8)
#define llGRZebesMapRidleyMObjSub ((intptr_t)0xCC90)
#define llGRZebesMapRidleyMatAnimJoint ((intptr_t)0xD030)
#define llGRZebesMapShipAnimJoint ((intptr_t)0xDEB0)
#define llGRZebesMapShipDObjDesc ((intptr_t)0xDE28)
#define llGRZebesMapShipMObjSub ((intptr_t)0xDAE0)
#define llGRZebesMapShipMatAnimJoint ((intptr_t)0xDF10)
#define llGRPupupuTestMapMapHeader ((intptr_t)0x14)
#define llGRCastleMapMapHeader ((intptr_t)0x14)
#define llGRCastleMapMapHead ((intptr_t)0x0)
#define llGRCastleMapDataStart ((intptr_t)0x2A08)
#define llGRCastleMapLakituDObjDesc ((intptr_t)0x4118)
#define llGRCastleMapLakituRAnimJoint ((intptr_t)0x4220)
#define llGRCastleMapLakituLAnimJoint ((intptr_t)0x4370)
#define llGRInishieMapMapHeader ((intptr_t)0x14)
#define llGRInishieMapScaleDObjDesc ((intptr_t)0x380)
#define llGRInishieMapMapHead ((intptr_t)0x5F0)
#define llGRInishieMapScaleRetractAnimJoint ((intptr_t)0x734)
#define llGRInishieMapPowerBlockGRAttackColl ((intptr_t)0x0BC)
#define llGRInishieMapPakkunItemAttributes ((intptr_t)0x120)
#define llGRInishieMapPakkunAppearAnimJoint ((intptr_t)0xCC8)
#define llGRInishieMapPakkunAppearMatAnimJoint ((intptr_t)0xCF8)
#define llGRInishieMapPakkunDamagedMatAnimJoint ((intptr_t)0xE04)
#define llGRInishieMapPowerBlockItemAttributes ((intptr_t)0xD8)
#define llGRInishieMapPowerBlockDataStart ((intptr_t)0x11F8)
#define llGRInishieMapPowerBlockAnimJoint ((intptr_t)0x1288)
#define llGRJungleMapMapHeader ((intptr_t)0x14)
#define llGRJungleMapTaruCannThrowHitDesc ((intptr_t)0xBC)
#define llGRJungleMapMapHead ((intptr_t)0xA98)
#define llGRJungleMapTaruCannDefaultAnimJoint ((intptr_t)0xB20)
#define llGRJungleMapTaruCannFillAnimJoint ((intptr_t)0xB68)
#define llGRJungleMapTaruCannShootAnimJoint ((intptr_t)0xBF8)
#define llGRJungleMapBirdMObjSub ((intptr_t)0xF230)
#define llGRJungleMapBirdDObjDesc ((intptr_t)0xF400)
#define llGRJungleMapBirdAnimJoint ((intptr_t)0xF490)
#define llGRJungleMapBirdMatAnimJoint ((intptr_t)0xF510)
#define llGRJungleMapDataStart ((intptr_t)0x9878)
#define llGRSectorMapMapHeader ((intptr_t)0x14)
#define llGRSectorMapMapHead ((intptr_t)0x0)
#define llGRSectorMapArwingLaser2DWeaponAttributes ((intptr_t)0xBC)
#define llGRSectorMapArwingLaser3DWeaponAttributes ((intptr_t)0xF0)
#define llGRSectorMapArwing0SectorDesc ((intptr_t)0x0000)
#define llGRSectorMapArwing1SectorDesc ((intptr_t)0x0250)
#define llGRSectorMapArwing2SectorDesc ((intptr_t)0x06D0)
#define llGRSectorMapArwing3SectorDesc ((intptr_t)0x03E0)
#define llGRSectorMapArwing4SectorDesc ((intptr_t)0x0D10)
#define llGRSectorMapArwing5SectorDesc ((intptr_t)0x0EB0)
#define llGRSectorMapArwing6SectorDesc ((intptr_t)0x1510)
#define llGRSectorMapArwing7SectorDesc ((intptr_t)0x11D0)
#define llGRSectorMapArwing0AnimJoint ((intptr_t)0x0000)
#define llGRSectorMapArwing1AnimJoint ((intptr_t)0x1D34)
#define llGRSectorMapArwing2AnimJoint ((intptr_t)0x1DA4)
#define llGRSectorMapArwing3AnimJoint ((intptr_t)0x1DC4)
#define llGRSectorMapArwing4AnimJoint ((intptr_t)0x1D54)
#define llGRSectorMapArwing5AnimJoint ((intptr_t)0x1DE4)
#define llGRSectorMapDataStart ((intptr_t)0x8680)
#define llGRSectorMapRocketAnimJoint ((intptr_t)0xADB0)
#define llGRSectorMapRocketDObjDesc ((intptr_t)0xAD00)
#define llGRSectorMapShipDObjDesc ((intptr_t)0xB6F8)
#define llGRSectorMapShipLAnimJoint ((intptr_t)0xB780)
#define llGRSectorMapShipMObjSub ((intptr_t)0xB3C0)
#define llGRSectorMapShipMatAnimJoint ((intptr_t)0xB7B0)
#define llGRSectorMapShipRAnimJoint ((intptr_t)0xB7E0)
#define llGRYosterMapMapHeader ((intptr_t)0x14)
#define llGRYosterMapMapHead ((intptr_t)0x100)
#define llGRYosterMap_1E0_AnimJoint ((intptr_t)0x1E0)
#define llGRYosterMap_4B8_MObjSub ((intptr_t)0x4B8)
#define llGRYosterMapCloudDisplayList ((intptr_t)0x580)
#define llGRYosterMapCloudSolidMatAnimJoint ((intptr_t)0x670)
#define llGRYosterMapCloudEvaporateMatAnimJoint ((intptr_t)0x690)
#define llGRYosterMapBirdAnimJoint ((intptr_t)0x7870)
#define llGRYosterMapBirdDObjDesc ((intptr_t)0x77E0)
#define llGRYosterMapBirdMObjSub ((intptr_t)0x7610)
#define llGRYosterMapBirdMatAnimJoint ((intptr_t)0x7950)
#define llGRYosterMapDataStart ((intptr_t)0x4E78)
#define llGRYosterMapHeihoFastAnimJoint ((intptr_t)0xB4E0)
#define llGRYosterMapHeihoFastDObjDesc ((intptr_t)0xB458)
#define llGRYosterMapHeihoFastMObjSub ((intptr_t)0xB290)
#define llGRYosterMapHeihoFastMatAnimJoint ((intptr_t)0xB550)
#define llGRYosterMapHeihoFruitFastAnimJoint ((intptr_t)0xA630)
#define llGRYosterMapHeihoFruitFastDObjDesc ((intptr_t)0xA5A8)
#define llGRYosterMapHeihoFruitFastMObjSub ((intptr_t)0xA3E0)
#define llGRYosterMapHeihoFruitFastMatAnimJoint ((intptr_t)0xA6A0)
#define llGRYosterMapHeihoFruitSlowAnimJoint ((intptr_t)0x8800)
#define llGRYosterMapHeihoFruitSlowMObjSub ((intptr_t)0x8360)
#define llGRYosterMapHeihoFruitSlowLDObjDesc ((intptr_t)0x8528)
#define llGRYosterMapHeihoFruitSlowMatAnimJoint ((intptr_t)0x8930)
#define llGRYosterMapHeihoFruitSlowRDObjDesc ((intptr_t)0x8778)
#define llGRYosterMapHeihoSlowAnimJoint ((intptr_t)0x9770)
#define llGRYosterMapHeihoSlowDObjDesc ((intptr_t)0x96E8)
#define llGRYosterMapHeihoSlowMObjSub ((intptr_t)0x9520)
#define llGRYosterMapHeihoSlowMatAnimJoint ((intptr_t)0x97F0)
#define llGRYamabukiMapMapHeader ((intptr_t)0x14)
#define llGRYamabukiMapItemHead ((intptr_t)0x14)
#define llGRYamabukiMapGLuckyItemAttributes ((intptr_t)0xBC)
#define llGRYamabukiMapMarumineItemAttributes ((intptr_t)0x104)
#define llGRYamabukiMapMarumineAttackEvents ((intptr_t)0x14C)
#define llGRYamabukiMapPorygonItemAttributes ((intptr_t)0x16C)
#define llGRYamabukiMapPorygonHitParties ((intptr_t)0x1B4)
#define llGRYamabukiMapHitokageItemAttributes ((intptr_t)0x1FC)
#define llGRYamabukiMapHitokageFlameWeaponAttributes ((intptr_t)0x244)
#define llGRYamabukiMapFushigibanaItemAttributes ((intptr_t)0x278)
#define llGRYamabukiMapFushigibanaHitParties ((intptr_t)0x2C0)
#define llGRYamabukiMapFushigibanaRazorWeaponAttributes ((intptr_t)0x308)
#define llGRYamabukiMapGateOpenAnimJoint ((intptr_t)0x9B0)
#define llGRYamabukiMapGateCloseAnimJoint ((intptr_t)0xA20)
#define llGRYamabukiMapMapHead ((intptr_t)0x8A0)
#define llGRYamabukiMapButterfreeAnimJoint ((intptr_t)0x99E0)
#define llGRYamabukiMapButterfreeDObjDesc ((intptr_t)0x9958)
#define llGRYamabukiMapButterfreeMObjSub ((intptr_t)0x9790)
#define llGRYamabukiMapButterfreeMatAnimJoint ((intptr_t)0x9DC0)
#define llGRYamabukiMapDataStart ((intptr_t)0x6A70)
#define llGRYamabukiMapFireAnimJoint ((intptr_t)0xC920)
#define llGRYamabukiMapFireDObjDesc ((intptr_t)0xC890)
#define llGRYamabukiMapFireMObjSub ((intptr_t)0xC6C0)
#define llGRYamabukiMapFireMatAnimJoint ((intptr_t)0xC9E0)
#define llGRYamabukiMapOnidrillAnimJoint ((intptr_t)0xE490)
#define llGRYamabukiMapOnidrillDObjDesc ((intptr_t)0xE400)
#define llGRYamabukiMapOnidrillMObjSub ((intptr_t)0xE230)
#define llGRYamabukiMapOnidrillMatAnimJoint ((intptr_t)0xE5C0)
#define llGRYamabukiMapPoppoAnimJoint ((intptr_t)0xFEE0)
#define llGRYamabukiMapPoppoDObjDesc ((intptr_t)0xFE58)
#define llGRYamabukiMapPoppoMObjSub ((intptr_t)0xFC90)
#define llGRYamabukiMapPoppoMatAnimJoint ((intptr_t)0xFFD0)
#define llGRHyruleMapMapHeader ((intptr_t)0x14)
#define llGRHyruleMapTwisterThrowHitDesc ((intptr_t)0xBC)
#define llGRLastMapMapHeader ((intptr_t)0x0)
#define llGRLastMapFileHead ((intptr_t)0x4D48)
#define llGRLastMapEffects0MObjSub ((intptr_t)0x086D8)
#define llGRLastMapEffects0DObjDesc ((intptr_t)0x08960)
#define llGRLastMapAnims0AnimJoint ((intptr_t)0x08A40)
#define llGRLastMapAnims0MatAnimJoint ((intptr_t)0x08C50)
#define llGRLastMapEffects1MObjSub ((intptr_t)0x097B0)
#define llGRLastMapEffects1DObjDesc ((intptr_t)0x0A188)
#define llGRLastMapAnims1AnimJoint ((intptr_t)0x0A340)
#define llGRLastMapAnims1MatAnimJoint ((intptr_t)0x0B1B0)
#define llGRLastMapEffects2_0MObjSub ((intptr_t)0x0D470)
#define llGRLastMapEffects2_0DObjDesc ((intptr_t)0x0DD90)
#define llGRLastMapEffects2_1MObjSub ((intptr_t)0x10788)
#define llGRLastMapEffects2_1DObjDesc ((intptr_t)0x11268)
#define llGRLastMapAnims2_0AnimJoint ((intptr_t)0x0DE70)
#define llGRLastMapAnims2_0MatAnimJoint ((intptr_t)0x0DEC0)
#define llGRLastMapAnims2_1MatAnimJoint ((intptr_t)0x11420)
#define llGRLastMapEffects3_0MObjSub ((intptr_t)0x10788)
#define llGRLastMapEffects3_0DObjDesc ((intptr_t)0x11268)
#define llGRLastMapEffects3_1DObjDesc ((intptr_t)0x12858)
#define llGRLastMapAnims3_0MatAnimJoint ((intptr_t)0x115C0)
#define llGRLastMapAnims3_1AnimJoint ((intptr_t)0x128E0)
#define llGRExplainMapMapHeader ((intptr_t)0x0)
#define llGRZakoMapMapHeader ((intptr_t)0x14)
#define llGRMetalMapMapHeader ((intptr_t)0x14)
#define llGRYosterSmallMapMapHeader ((intptr_t)0x14)
#define llGRBonus1MarioMapMapHeader ((intptr_t)0x0)
#define llGRBonus1MarioMapTargetsStart ((intptr_t)0x1EB0)
#define llGRBonus1MarioMapTargetsDObjDesc ((intptr_t)0x2150)
#define llGRBonus1MarioMapTargetsAnimJoint ((intptr_t)0x2360)
#define llGRBonus1FoxMapMapHeader ((intptr_t)0x0)
#define llGRBonus1FoxMapTargetsStart ((intptr_t)0x2068)
#define llGRBonus1FoxMapTargetsDObjDesc ((intptr_t)0x24B0)
#define llGRBonus1FoxMapTargetsAnimJoint ((intptr_t)0x26C0)
#define llGRBonus1DonkeyMapMapHeader ((intptr_t)0x0)
#define llGRBonus1DonkeyMapTargetsStart ((intptr_t)0x1F20)
#define llGRBonus1DonkeyMapTargetsDObjDesc ((intptr_t)0x2250)
#define llGRBonus1DonkeyMapTargetsAnimJoint ((intptr_t)0x2460)
#define llGRBonus1SamusMapMapHeader ((intptr_t)0x0)
#define llGRBonus1SamusMapTargetsStart ((intptr_t)0x1868)
#define llGRBonus1SamusMapTargetsDObjDesc ((intptr_t)0x1B30)
#define llGRBonus1SamusMapTargetsAnimJoint ((intptr_t)0x1D40)
#define llGRBonus1LuigiMapMapHeader ((intptr_t)0x0)
#define llGRBonus1LuigiMapTargetsStart ((intptr_t)0x1BA0)
#define llGRBonus1LuigiMapTargetsDObjDesc ((intptr_t)0x2020)
#define llGRBonus1LuigiMapTargetsAnimJoint ((intptr_t)0x2230)
#define llGRBonus1LinkMapMapHeader ((intptr_t)0x0)
#define llGRBonus1LinkMapTargetsStart ((intptr_t)0x2378)
#define llGRBonus1LinkMapTargetsDObjDesc ((intptr_t)0x2770)
#define llGRBonus1LinkMapTargetsAnimJoint ((intptr_t)0x2980)
#define llGRBonus1YoshiMapMapHeader ((intptr_t)0x0)
#define llGRBonus1YoshiMapTargetsStart ((intptr_t)0x2D68)
#define llGRBonus1YoshiMapTargetsDObjDesc ((intptr_t)0x3290)
#define llGRBonus1YoshiMapTargetsAnimJoint ((intptr_t)0x34A0)
#define llGRBonus1CaptainMapMapHeader ((intptr_t)0x0)
#define llGRBonus1CaptainMapTargetsStart ((intptr_t)0x1888)
#define llGRBonus1CaptainMapTargetsDObjDesc ((intptr_t)0x1B70)
#define llGRBonus1CaptainMapTargetsAnimJoint ((intptr_t)0x1D80)
#define llGRBonus1KirbyMapMapHeader ((intptr_t)0x0)
#define llGRBonus1KirbyMapTargetsStart ((intptr_t)0x2150)
#define llGRBonus1KirbyMapTargetsDObjDesc ((intptr_t)0x2510)
#define llGRBonus1KirbyMapTargetsAnimJoint ((intptr_t)0x2720)
#define llGRBonus1PikachuMapMapHeader ((intptr_t)0x0)
#define llGRBonus1PikachuMapTargetsStart ((intptr_t)0x2658)
#define llGRBonus1PikachuMapTargetsDObjDesc ((intptr_t)0x2A70)
#define llGRBonus1PikachuMapTargetsAnimJoint ((intptr_t)0x2C80)
#define llGRBonus1PurinMapMapHeader ((intptr_t)0x0)
#define llGRBonus1PurinMapTargetsStart ((intptr_t)0x1FF8)
#define llGRBonus1PurinMapTargetsDObjDesc ((intptr_t)0x23A0)
#define llGRBonus1PurinMapTargetsAnimJoint ((intptr_t)0x25B0)
#define llGRBonus1NessMapMapHeader ((intptr_t)0x0)
#define llGRBonus1NessMapTargetsStart ((intptr_t)0x2940)
#define llGRBonus1NessMapTargetsDObjDesc ((intptr_t)0x2E60)
#define llGRBonus1NessMapTargetsAnimJoint ((intptr_t)0x3070)
#define llGRBonus2MarioMapMapHeader ((intptr_t)0x0)
#define llGRBonus2FoxMapMapHeader ((intptr_t)0x0)
#define llGRBonus2FoxMapBumpersDObjDesc ((intptr_t)0xE160)
#define llGRBonus2FoxMapBumpersAnimJoint ((intptr_t)0xE350)
#define llGRBonus2DonkeyMapMapHeader ((intptr_t)0x0)
#define llGRBonus2SamusMapMapHeader ((intptr_t)0x0)
#define llGRBonus2SamusMapBumpersDObjDesc ((intptr_t)0x2910)
#define llGRBonus2SamusMapBumpersAnimJoint ((intptr_t)0x29C0)
#define llGRBonus2LuigiMapMapHeader ((intptr_t)0x0)
#define llGRBonus2LinkMapMapHeader ((intptr_t)0x0)
#define llGRBonus2YoshiMapMapHeader ((intptr_t)0x0)
#define llGRBonus2CaptainMapMapHeader ((intptr_t)0x0)
#define llGRBonus2KirbyMapMapHeader ((intptr_t)0x0)
#define llGRBonus2KirbyMapBumpersDObjDesc ((intptr_t)0x3920)
#define llGRBonus2KirbyMapBumpersAnimJoint ((intptr_t)0x3A60)
#define llGRBonus2PikachuMapMapHeader ((intptr_t)0x0)
#define llGRBonus2PurinMapMapHeader ((intptr_t)0x0)
#define llGRBonus2PurinMapBumpersDObjDesc ((intptr_t)0x4FE0)
#define llGRBonus2PurinMapBumpersAnimJoint ((intptr_t)0x5120)
#define llGRBonus2NessMapMapHeader ((intptr_t)0x0)
#define llGRBonus2NessMapBumpersDObjDesc ((intptr_t)0x3FE0)
#define llGRBonus2NessMapBumpersAnimJoint ((intptr_t)0x4090)
#define llGRBonus3MapMapHeader ((intptr_t)0x0)
#define llGRBonus3MapMapHead ((intptr_t)0x0)
#define llGRBonus3MapItemHead ((intptr_t)0x0)
#define llGRBonus3MapBumpersDObjDesc ((intptr_t)0x0)
#define llGRBonus3MapTaruBombItemAttributes ((intptr_t)0xA8)
#define llGRBonus3MapTaruBombAttackEvents ((intptr_t)0xF0)
#define llGRBonus3MapBumpersAnimJoint ((intptr_t)0x110)
#define llGRBonus3MapTaruBombDataStart ((intptr_t)0x788)
#define llGRBonus3MapTaruBombEffectDisplayList ((intptr_t)0x8A0)
#define llMarioModelStockSprite ((intptr_t)0x72d0)
#define llMarioModelFTEmblemSprite ((intptr_t)0x74c8)
#define llMMarioModelStockSprite ((intptr_t)0x2d00)
#define llMMarioModelFTEmblemSprite ((intptr_t)0x2ef8)
#define llFoxModelStockSprite ((intptr_t)0x7c28)
#define llFoxModelFTEmblemSprite ((intptr_t)0x7e08)
#define llDkIconStockSprite ((intptr_t)0x130)
#define llDkIconFTEmblemSprite ((intptr_t)0x2f8)
#define llSamusModelStockSprite ((intptr_t)0xe2f0)
#define llSamusModelFTEmblemSprite ((intptr_t)0xe508)
#define llLuigiModelStockSprite ((intptr_t)0x7cd8)
#define llLuigiModelFTEmblemSprite ((intptr_t)0x7ec8)
#define llLinkModelStockSprite ((intptr_t)0x11d48)
#define llLinkModelFTEmblemSprite ((intptr_t)0x11f28)
#define llKirbyModelStockSprite ((intptr_t)0x1d5e0)
#define llKirbyModelFTEmblemSprite ((intptr_t)0x1d7d8)
#define llPurinModelStockSprite ((intptr_t)0x7bb0)
#define llPurinModelFTEmblemSprite ((intptr_t)0x7d98)
#define llCaptainModelStockSprite ((intptr_t)0xc6a8)
#define llCaptainModelFTEmblemSprite ((intptr_t)0xc848)
#define llCaptainSpecial3FalconPunchDObjDesc ((intptr_t)0x0760)
#define llCaptainSpecial3FalconPunchMObjSub ((intptr_t)0x0690)
#define llCaptainSpecial3FalconPunchMatAnimJoint ((intptr_t)0x0830)
#define llNessModelPKThunderTrailDObjDesc ((intptr_t)0x9050)
#define llNessModelPKThunderWaveMObjSub ((intptr_t)0x9870)
#define llNessModelPKThunderWaveDObjDesc ((intptr_t)0x9A10)
#define llNessModelPKThunderWaveAnimJoint ((intptr_t)0x9AC0)
#define llNessModelPKThunderWaveMatAnimJoint ((intptr_t)0x9BB0)
#define llNessModelStockSprite ((intptr_t)0xc188)
#define llNessModelFTEmblemSprite ((intptr_t)0xc378)
#define llYoshiModelStockSprite ((intptr_t)0xaaa8)
#define llYoshiModelShieldDObjDesc ((intptr_t)0xa860)
#define llYoshiModelFTEmblemSprite ((intptr_t)0xac98)
#define llYoshiSpecial3EggLayDObjDesc ((intptr_t)0x960)
#define llYoshiSpecial3EggLayBreakAnimJoint ((intptr_t)0x9F0)
#define llYoshiSpecial3EggLayThrowAnimJoint ((intptr_t)0xB90)
#define llYoshiSpecial3EggLayWaitAnimJoint ((intptr_t)0xDB0)
#define llPikachuModelThunderTrailMObjSub ((intptr_t)0x9420)
#define llPikachuModelThunderTrailDObjDesc ((intptr_t)0x95B0)
#define llPikachuModelStockSprite ((intptr_t)0x9a00)
#define llPikachuModelFTEmblemSprite ((intptr_t)0x9be8)
#define llPikachuSpecial3ThunderJoltBAnimJoint ((intptr_t)0x1A20)
#define llPikachuSpecial3ThunderJoltBMatAnimJoint ((intptr_t)0x1AE0)
#define llPikachuSpecial3ThunderJoltDObjDesc ((intptr_t)0x2258)
#define llPikachuSpecial3ThunderJoltMObjSub ((intptr_t)0x20A0)
#define llPikachuSpecial3ThunderJoltAnimJoint ((intptr_t)0x22E0)
#define llPikachuSpecial3ThunderJoltMatAnimJoint ((intptr_t)0x2350)
#define llMasterHandIconStockSprite ((intptr_t)0xc0)
#define llMasterHandIconFTEmblemSprite ((intptr_t)0x2b8)
#define llFoxSpecial2ReflectorDObjDesc ((intptr_t)0x2B0)
#define llFoxSpecial2EntryArwingLAnimJoint ((intptr_t)0x590)
#define llFoxSpecial2EntryArwingRAnimJoint ((intptr_t)0x9E0)
#define llFoxSpecial2ReflectorStartAnimJoint ((intptr_t)0x340)
#define llFoxSpecial2ReflectorHitAnimJoint ((intptr_t)0x3A0)
#define llFoxSpecial2ReflectorEndAnimJoint ((intptr_t)0x430)
#define llFoxSpecial2ReflectorLoopAnimJoint ((intptr_t)0x4C0)
#define llPikachuSpecial2UnkMObjSub ((intptr_t)0x0640)
#define llPikachuSpecial2UnkDObjDesc ((intptr_t)0x0800)
#define llPikachuSpecial2UnkAnimJoint ((intptr_t)0x0890)
#define llPikachuSpecial2UnkMatAnimJoint ((intptr_t)0x0900)
#define llPikachuSpecial2ThunderShockMObjSub ((intptr_t)0x13A0)
#define llPikachuSpecial2ThunderShockDObjDesc ((intptr_t)0x1640)
#define llPikachuSpecial2ThunderShock0AnimJoint ((intptr_t)0x1720)
#define llPikachuSpecial2ThunderShock1AnimJoint ((intptr_t)0x1850)
#define llPikachuSpecial2ThunderShock2AnimJoint ((intptr_t)0x1970)
#define llPikachuSpecial2ThunderShock0MatAnimJoint ((intptr_t)0x1A80)
#define llPikachuSpecial2ThunderShock1MatAnimJoint ((intptr_t)0x1AC0)
#define llPikachuSpecial2ThunderShock2MatAnimJoint ((intptr_t)0x1B10)
#define llKirbySpecial2VulcanJabDObjDesc ((intptr_t)0x0B20)
#define llKirbySpecial2CutterUpDObjDesc ((intptr_t)0x12E8)
#define llKirbySpecial2CutterUpAnimJoint ((intptr_t)0x1470)
#define llKirbySpecial2CutterDownDObjDesc ((intptr_t)0x2390)
#define llKirbySpecial2CutterDownAnimJoint ((intptr_t)0x24D0)
#define llKirbySpecial2CutterDrawDObjDesc ((intptr_t)0x2888)
#define llKirbySpecial2CutterTrailDObjDesc ((intptr_t)0x0DF8)
#define llKirbySpecial2CutterTrailAnimJoint ((intptr_t)0x13F0)
#define llKirbySpecial2EntryStarDObjDesc ((intptr_t)0x1DA8)
#define llKirbySpecial2EntryStarLAnimJoint ((intptr_t)0x1E30)
#define llKirbySpecial2EntryStarRAnimJoint ((intptr_t)0x1EA0)
#define llSamusSpecial2GrappleBeamDObjDesc ((intptr_t)0x0380)
#define llSamusSpecial2GrappleBeamMObjSub ((intptr_t)0x0210)
#define llSamusSpecial2GrappleBeamAnimJoint ((intptr_t)0x0410)
#define llSamusSpecial2GrappleBeamMatAnimJoint ((intptr_t)0x0480)
#define llSamusSpecial2EntryPointDObjDesc ((intptr_t)0x0B90)
#define llSamusSpecial2EntryPointAnimJoint ((intptr_t)0x0C20)
#define llCaptainSpecial2FalconKickDObjDesc ((intptr_t)0x0B08)
#define llCaptainSpecial2FalconKickMObjSub ((intptr_t)0x0960)
#define llCaptainSpecial2FalconKickAnimJoint ((intptr_t)0x0B90)
#define llCaptainSpecial2FalconKickMatAnimJoint ((intptr_t)0x0C00)
#define llCaptainSpecial2EntryCarDObjDesc ((intptr_t)0x5FC0)
#define llCaptainSpecial2_6200_AnimJoint ((intptr_t)0x6200)
#define llCaptainSpecial2_6518_AnimJoint ((intptr_t)0x6518)
#define llCaptainSpecial2_6598_AnimJoint ((intptr_t)0x6598)
#define llPurinSpecial2SingDObjDesc ((intptr_t)0x2130)
#define llPurinSpecial2SingMObjSub ((intptr_t)0x1C20)
#define llPurinSpecial2SingAnimJoint ((intptr_t)0x2270)
#define llPurinSpecial2SingMatAnimJoint ((intptr_t)0x2D70)
#define llNessSpecial2PsychicMagnetMObjSub ((intptr_t)0x0810)
#define llNessSpecial2PsychicMagnetDObjDesc ((intptr_t)0x09A8)
#define llNessSpecial2PsychicMagnetAnimJoint ((intptr_t)0x0A30)
#define llNessSpecial2PsychicMagnetMatAnimJoint ((intptr_t)0x0AD0)
#define llLinkSpecial2EntryWaveDObjDesc ((intptr_t)0x03F8)
#define llLinkSpecial2EntryWaveMObjSub ((intptr_t)0x0130)
#define llLinkSpecial2EntryWaveAnimJoint ((intptr_t)0x0840)
#define llLinkSpecial2EntryWaveMatAnimJoint ((intptr_t)0x0B90)
#define llLinkSpecial2EntryBeamDObjDesc ((intptr_t)0x07B8)
#define llLinkSpecial2EntryBeamMObjSub ((intptr_t)0x04F0)
#define llLinkSpecial2EntryBeamAnimJoint ((intptr_t)0x0B60)
#define llLinkSpecial2EntryBeamMatAnimJoint ((intptr_t)0x0BF0)
#define llLinkSpecial2SpinAttackDObjDesc ((intptr_t)0x11C0)
#define llLinkSpecial2SpinAttackMObjSub ((intptr_t)0x1038)
#define llLinkSpecial2SpinAttackAnimJoint ((intptr_t)0x1250)
#define llLinkSpecial2SpinAttackMatAnimJoint ((intptr_t)0x12F0)
#define llYoshiSpecial2EntryEggDObjDesc ((intptr_t)0x0530)
#define llYoshiSpecial2EntryEggMObjSub ((intptr_t)0x0460)
#define llYoshiSpecial2EntryEggAnimJoint ((intptr_t)0x0600)
#define llYoshiSpecial2EntryEggMatAnimJoint ((intptr_t)0x0780)
#define llDonkeySpecial2EntryTaruDObjDesc ((intptr_t)0x07C8)
#define llDonkeySpecial2EntryTaruAnimJoint ((intptr_t)0x0850)
#define llMarioSpecial2EntryDokanDObjDesc ((intptr_t)0x0608)
#define llMarioSpecial2EntryDokanAnimJoint ((intptr_t)0x06C0)

/* ll* symbols referenced by src/ but absent from the symbols table. */
#define llFTBossAnimAppearFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimChargeStart1FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimChargeStart2FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimDefaultFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimDrillFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimDyingFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimDyingStartFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimFlickFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimFlyFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimGunAimFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimGunDrawFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimGunShootFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimLanding2FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimLaunchFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPointFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPointStartFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPokeFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPunch1FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPunch2FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPunch3FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimPunchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimShooFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimSlamFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimSlamStartFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimSlapFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimUnknown1FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimUnknown6FileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTBossAnimWalkStartFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAppearAirFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAppearFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimBlueFalcon1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimBlueFalcon2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCatchingEnemyWhileDivingFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownSpecialAirFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimDownSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFSmashHighFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFSmashLowFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFTiltMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFTiltMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFalconDiveEnd1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFalconDiveEnd2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFalconDiveFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFalconPunchAirFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFalconPunchGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJab3FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJabLoopEndFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJabLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJabLoopStartFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimLandingDownSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimVelocityXDownSpecialAirFileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimVictory1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimVictory2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTCaptainAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCargoAirThrowFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCargoLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCargoSlowWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCargoTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCargoVerySlowWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCargoWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFSmashHighFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFSmashLowFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFSmashMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFSmashMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimGiantPunchAirFullyChargedPunchFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimGiantPunchAirLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimGiantPunchAirLoopStartFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimGiantPunchGroundFullyChargedPunchFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimGiantPunchGroundLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimGiantPunchGroundLoopStartFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimHandSlapEndFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimHandSlapLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimHandSlapStartFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimIntroFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimSpinningKongAirFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimSpinningKongGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrownFoxFStartFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTDonkeyAnimWin3FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimAppearFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimArwingFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDollFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFTiltMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFTiltMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFoxAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFoxEndAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFoxEndGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFoxGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFoxStartAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimFireFoxStartGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJabLoopEndFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJabLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJabLoopStartFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLandingWhileFireFoxAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLaserAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLaserFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimPose1FrameFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimPoseBlaster2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimPosePostBlasterFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimReadyingFireFoxAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimReadyingFireFoxGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimReflectingFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShineAirEndFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShineFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShineStartFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimShireStartAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimSwitchDirectionShineAirFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimSwitchDirectionShineFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrown3FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrownFoxBFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTFoxAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTGDonkeyAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimBoomerangAirCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimBoomerangAirMissFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimBoomerangCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimBoomerangMissFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargePunchAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargePunchAirFullFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargePunchGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargePunchGroundFullFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargePunchStartGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargeShotAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargeShotStartFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargeStartAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimChargingFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDKStaringAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDKStaringGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEggLayGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEggThrowAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEggThrowEndAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEggThrowGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFalconPunchAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFalconPunchGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFinalCutterFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFinalCutterImpactFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFinalCutterLandFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimForwardThrowFallFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimForwardThrowFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimForwardThrowRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleAbsorbFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleEndFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleSpitFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleStartGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleStuffedFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleSwallowedFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimInhaleTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimIntro2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimIntroFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimIntroPoseFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJabLoopEndFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJabLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJabLoopStartFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJump2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJump3FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJump4FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJump5FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJump6FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLaserAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLaserGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLuigiFireballAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimLuigiFireballGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimPKFireAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimPKFireGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimPose2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimPoseFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimShootingChargeShotAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimShootingChargeShotFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStoneCancelFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStoneGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStoneStartAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStoneStartGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrown3FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrownFoxBFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThunderJoltAirFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimThunderJoltGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTKirbyAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimBombAirFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimBombFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCatchingBoomerangAirFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCatchingBoomerangFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimClaps2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimIntroFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJab3FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJabLoopEndFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJabLoopFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJabLoopStartFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimLandingAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimMissingBoomerangAirFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimMissingBoomerangFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimPoseAllyFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimPoseOpponentFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimPosePlayerFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrownFoxFStartFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimUpSpecialAirFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimUpSpecialEndGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimUpSpecialGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTLinkAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimFSmashHighFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimFSmashLowFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimFSmashMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimFSmashMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimPoseEnemyFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimPosePlayerFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTLuigiAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTMMarioAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDollFallsFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFSmashHighFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFSmashLowFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFSmashMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFSmashMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFireballAirFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimFireballGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimIntro2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimIntroFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimIntroVsKirbyFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJab3FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimLandingAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimMarioTornadoAirFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimMarioTornadoGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimPose2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimPoseFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimSuperJumpPunchAirFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWin3FileID ((intptr_t)0) /* STUBBED */
#define llFTMarioAnimWinBetaFileID ((intptr_t)0) /* STUBBED */
#define llFTNCaptainAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNDonkeyAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNFoxAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNKirbyAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNLinkAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNMarioAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNNessAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNPikachuAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNSamusAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNYoshiAnimPose1PFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAppearEndFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAppearStart1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAppearStart2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimClashingDuringPKTAFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDKTAAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownBStartGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownSpecialEndAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownSpecialEndGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimDownSpecialStartAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimHealingAirDownBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimHealingDownBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJab3FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKFireAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKFireGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKThunderAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKThunderEndAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKThunderEndFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKThunderStartAirFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKThunderStartGround1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPKThunderStartGround2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimPosePlayerFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTNessAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownSpecialEndAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownSpecialEndFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownSpecialStartAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownSpecialStartFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimDownSpecialThunderedAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimGettingThunderedFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimLandingAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimNeutralSpecialAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimNeutralSpecialGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimPoseAllyFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimPoseEnemyFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimPosePlayerFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimUnknown1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimUnknown2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimUpSpecialAirEndFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimUpSpecialEndFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTPikachuAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimBatDashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimBatSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimHeavyItemThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimItemThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJump2FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJump3FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJump4FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJump5FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJump6FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimJumpSquatFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimPassFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimPoundAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimPoundGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimRestAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimSingAirFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTPurinAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimBombAirFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimBombFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimChargingNeutralSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimClapsFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFSmashHighFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFSmashLowFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFSmashMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFSmashMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFTiltMidHighFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFTiltMidLowFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimIntroFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimJumpAerialFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimLandingAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimPoseEnemyFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimPosePlayerFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimScrewAttackAirFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimScrewAttackGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimShootingAirFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimShootingFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStartingChargeShotAirFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStartingChargeShotFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrown3FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrownFoxBFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrownFoxFStartFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimUnknownFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTSamusAnimWin3FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAppear1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAppear2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAttackAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAttackAirDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAttackAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAttackAirNFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimAttackAirUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCatchPullFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCeilingBonkFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimClangRecoilFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffAttackQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffAttackQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffAttackSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffAttackSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffCatchFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffClimbQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffClimbQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffClimbSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffClimbSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffEscapeQuick1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffEscapeQuick2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffEscapeSlow1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffEscapeSlow2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffQuickFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffSlowFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCliffWaitFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCrouchEndFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCrouchFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimCrouchIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamage2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageAirFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageFlyTopFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageFlyX1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageFlyX2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageX1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageX2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamageX3FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged3FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged4FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged5FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged6FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamaged7FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDamagedFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDashAttackFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDollFallFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDollPickUpFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDollRevivalFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDownAttackDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDownAttackUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDownBackDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDownBackUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDownForwardDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimDownForwardUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggLayAirGrabInFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggLayAirGrabOutFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggLayFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggLayGrabbedSomeoneComingInAndSwallowingFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggLayGrabbedSomeoneStillGoingOutFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggLayPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggThrowAirFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEggThrowGroundFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimEnterPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimExitPipeFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimExitPipeWalkFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFSmashHighFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFSmashLowFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFTiltHighFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFTiltLowFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFalconDivePulledFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFallAerialFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFallFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFallSpecialFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFireFlowerShootAirFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimFireFlowerShootFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimGroundPoundAirFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimGroundPoundGroundStartFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimGroundPoundLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimHammerIdleFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimHammerLandingFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimHeavyItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimHeavyItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimIntroPoseFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemDropFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowAirSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowAirSmashFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowAirSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowDashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowSmashBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowSmashDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimItemThrowSmashUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimJab1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimJab2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimJumpAerialBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimJumpBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimJumpFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimLandingAirBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimLandingAirFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimLandingAirXFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimLightItemPickupFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimRollBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimRollFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimRunBrakeFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimRunFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimSelectedFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimShieldBreakFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimShieldDropFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimShieldOffFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimShieldOnFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimSleepFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStarRodDashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStarRodNeutralFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStarRodSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStarRodTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStunLandDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStunLandUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStunStartDFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimStunStartUFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTauntFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTeamPosesFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTechBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTechFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTechFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTeeterFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTeeterstartFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrowBFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrowFFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrown1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrown2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrownDKFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrownDKPulledFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimThrownMarioBrosFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTurnFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimTurnRunFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUSmashFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUTiltFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown10FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown3FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown4FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown5FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown6FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown7FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown8FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimUnknown9FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimWalk1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimWalk2FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimWalk3FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimWalkEndFileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimWin1FileID ((intptr_t)0) /* STUBBED */
#define llFTYoshiAnimWin2FileID ((intptr_t)0) /* STUBBED */
#define llMNTitleUnknownAnimJoint ((intptr_t)0) /* STUBBED */
#define llMNTitleUnknownDObjDesc ((intptr_t)0) /* STUBBED */

#endif /* _RELOC_DATA_H_ */
