/* 
 * CloudSeed
 * Ported from https://github.com/GhostNoteAudio/CloudSeedCore
 */

#pragma once

#define BUFFER_SIZE 128
#define SLOW_CLEAR_SIZE 192000 // may need to be adjusted for other Pis

#include "../CloudSeedCore/DSP/ReverbController.h"
#include <atomic>

namespace Parameter = Cloudseed::Parameter;

static float FXDivineInspiration[Parameter::COUNT];
static float FXLawsOfPhysics[Parameter::COUNT];
static float FXSlowBraaam[Parameter::COUNT];
static float FXTheUpsideDown[Parameter::COUNT];
static float LBigSoundStage[Parameter::COUNT];
static float LDiffusionCyclone[Parameter::COUNT];
static float LScreamIntoTheVoid[Parameter::COUNT];
static float M90sDigitalReverb[Parameter::COUNT];
static float MAiryAmbience[Parameter::COUNT];
static float MDarkPlate[Parameter::COUNT];
static float MGhostly[Parameter::COUNT];
static float MTappedLines[Parameter::COUNT];
static float SFastAttack[Parameter::COUNT];
static float SSmallPlate[Parameter::COUNT];
static float SSnappyAttack[Parameter::COUNT];

static float *Presets[] = {
        FXDivineInspiration,
        FXLawsOfPhysics,
        FXSlowBraaam,
        FXTheUpsideDown,
        LBigSoundStage,
        LDiffusionCyclone,
        LScreamIntoTheVoid,
        M90sDigitalReverb,
        MAiryAmbience,
        MDarkPlate,
        MGhostly,
        MTappedLines,
        SFastAttack,
        SSmallPlate,
        SSnappyAttack,
};

class AudioEffectCloudSeed
{
public:
    static int GetPresetNum() { return sizeof Presets / sizeof *Presets; }

    AudioEffectCloudSeed(float samplerate):
    samplerate{samplerate},
    ramp_dt{10.0f / samplerate},
    engine{(int)samplerate},
    enabled{},
    targetVol{},
    needBufferClear{},
    waitBufferClear{},
    needParameterLoad{},
    preset{},
    vol{}
    {
        FXDivineInspiration[Parameter::DryOut]=0.0;
        FXDivineInspiration[Parameter::EarlyDiffuseCount]=0.7172999978065491;
        FXDivineInspiration[Parameter::EarlyDiffuseDelay]=0.7386999726295471;
        FXDivineInspiration[Parameter::EarlyDiffuseEnabled]=1.0;
        FXDivineInspiration[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        FXDivineInspiration[Parameter::EarlyDiffuseModAmount]=0.2505999803543091;
        FXDivineInspiration[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        FXDivineInspiration[Parameter::EarlyOut]=0.0;
        FXDivineInspiration[Parameter::EqCrossSeed]=0.1119999960064888;
        FXDivineInspiration[Parameter::EqCutoff]=0.8172999620437622;
        FXDivineInspiration[Parameter::EqHighFreq]=0.5346999764442444;
        FXDivineInspiration[Parameter::EqHighGain]=0.543999969959259;
        FXDivineInspiration[Parameter::EqHighShelfEnabled]=0.0;
        FXDivineInspiration[Parameter::EqLowFreq]=0.3879999816417694;
        FXDivineInspiration[Parameter::EqLowGain]=0.5559999942779541;
        FXDivineInspiration[Parameter::EqLowShelfEnabled]=0.0;
        FXDivineInspiration[Parameter::EqLowpassEnabled]=1.0;
        FXDivineInspiration[Parameter::LowCut]=0.2933000028133392;
        FXDivineInspiration[Parameter::LowCutEnabled]=0.0;
        FXDivineInspiration[Parameter::InputMix]=0.2346999943256378;
        FXDivineInspiration[Parameter::Interpolation]=0.0;
        FXDivineInspiration[Parameter::LateDiffuseCount]=0.4786999821662903;
        FXDivineInspiration[Parameter::LateDiffuseDelay]=0.6279999613761902;
        FXDivineInspiration[Parameter::LateDiffuseEnabled]=1.0;
        FXDivineInspiration[Parameter::LateDiffuseFeedback]=0.6319999694824219;
        FXDivineInspiration[Parameter::LateDiffuseModAmount]=0.4614000022411346;
        FXDivineInspiration[Parameter::LateDiffuseModRate]=0.3267000019550323;
        FXDivineInspiration[Parameter::LateLineCount]=1.0;
        FXDivineInspiration[Parameter::LateLineDecay]=0.8199999928474426;
        FXDivineInspiration[Parameter::LateLineModAmount]=0.7547000050544739;
        FXDivineInspiration[Parameter::LateLineModRate]=0.6279999613761902;
        FXDivineInspiration[Parameter::LateLineSize]=0.687999963760376;
        FXDivineInspiration[Parameter::LateMode]=1.0;
        FXDivineInspiration[Parameter::LateOut]=0.7786999940872192;
        FXDivineInspiration[Parameter::HighCut]=1.0;
        FXDivineInspiration[Parameter::HighCutEnabled]=0.0;
        FXDivineInspiration[Parameter::SeedDelay]=0.2309999912977219;
        FXDivineInspiration[Parameter::SeedDiffusion]=0.1469999998807907;
        FXDivineInspiration[Parameter::SeedPostDiffusion]=0.2899999916553497;
        FXDivineInspiration[Parameter::SeedTap]=0.3339999914169312;
        FXDivineInspiration[Parameter::TapDecay]=1.0;
        FXDivineInspiration[Parameter::TapLength]=0.9866999983787537;
        FXDivineInspiration[Parameter::TapPredelay]=0.1599999964237213;
        FXDivineInspiration[Parameter::TapCount]=0.1959999948740005;
        FXDivineInspiration[Parameter::TapEnabled]=0.0;

        FXLawsOfPhysics[Parameter::DryOut]=1.0;
        FXLawsOfPhysics[Parameter::EarlyDiffuseCount]=1.0;
        FXLawsOfPhysics[Parameter::EarlyDiffuseDelay]=0.7706999778747559;
        FXLawsOfPhysics[Parameter::EarlyDiffuseEnabled]=1.0;
        FXLawsOfPhysics[Parameter::EarlyDiffuseFeedback]=0.5026999711990356;
        FXLawsOfPhysics[Parameter::EarlyDiffuseModAmount]=0.6158999800682068;
        FXLawsOfPhysics[Parameter::EarlyDiffuseModRate]=0.3666999936103821;
        FXLawsOfPhysics[Parameter::EarlyOut]=0.7999999523162842;
        FXLawsOfPhysics[Parameter::EqCrossSeed]=0.2680000066757202;
        FXLawsOfPhysics[Parameter::EqCutoff]=0.7439999580383301;
        FXLawsOfPhysics[Parameter::EqHighFreq]=0.5133999586105347;
        FXLawsOfPhysics[Parameter::EqHighGain]=0.7680000066757202;
        FXLawsOfPhysics[Parameter::EqHighShelfEnabled]=0.0;
        FXLawsOfPhysics[Parameter::EqLowFreq]=0.4079999923706055;
        FXLawsOfPhysics[Parameter::EqLowGain]=0.5559999942779541;
        FXLawsOfPhysics[Parameter::EqLowShelfEnabled]=0.0;
        FXLawsOfPhysics[Parameter::EqLowpassEnabled]=0.0;
        FXLawsOfPhysics[Parameter::LowCut]=0.1772999912500381;
        FXLawsOfPhysics[Parameter::LowCutEnabled]=1.0;
        FXLawsOfPhysics[Parameter::InputMix]=0.2800000011920929;
        FXLawsOfPhysics[Parameter::Interpolation]=0.0;
        FXLawsOfPhysics[Parameter::LateDiffuseCount]=0.4879999756813049;
        FXLawsOfPhysics[Parameter::LateDiffuseDelay]=0.239999994635582;
        FXLawsOfPhysics[Parameter::LateDiffuseEnabled]=0.0;
        FXLawsOfPhysics[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        FXLawsOfPhysics[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        FXLawsOfPhysics[Parameter::LateDiffuseModRate]=0.1666999906301498;
        FXLawsOfPhysics[Parameter::LateLineCount]=0.6039999723434448;
        FXLawsOfPhysics[Parameter::LateLineDecay]=0.5759999752044678;
        FXLawsOfPhysics[Parameter::LateLineModAmount]=0.2719999849796295;
        FXLawsOfPhysics[Parameter::LateLineModRate]=0.0372999981045723;
        FXLawsOfPhysics[Parameter::LateLineSize]=0.3240000009536743;
        FXLawsOfPhysics[Parameter::LateMode]=1.0;
        FXLawsOfPhysics[Parameter::LateOut]=0.6439999938011169;
        FXLawsOfPhysics[Parameter::HighCut]=1.0;
        FXLawsOfPhysics[Parameter::HighCutEnabled]=1.0;
        FXLawsOfPhysics[Parameter::SeedDelay]=0.2180999964475632;
        FXLawsOfPhysics[Parameter::SeedDiffusion]=0.1850000023841858;
        FXLawsOfPhysics[Parameter::SeedPostDiffusion]=0.3652999997138977;
        FXLawsOfPhysics[Parameter::SeedTap]=0.3489999771118164;
        FXLawsOfPhysics[Parameter::TapDecay]=0.5040000081062317;
        FXLawsOfPhysics[Parameter::TapLength]=1.0;
        FXLawsOfPhysics[Parameter::TapPredelay]=0.0;
        FXLawsOfPhysics[Parameter::TapCount]=1.0;
        FXLawsOfPhysics[Parameter::TapEnabled]=1.0;

        FXSlowBraaam[Parameter::DryOut]=0.0;
        FXSlowBraaam[Parameter::EarlyDiffuseCount]=0.7759999632835388;
        FXSlowBraaam[Parameter::EarlyDiffuseDelay]=0.4986999928951263;
        FXSlowBraaam[Parameter::EarlyDiffuseEnabled]=1.0;
        FXSlowBraaam[Parameter::EarlyDiffuseFeedback]=0.5467000007629395;
        FXSlowBraaam[Parameter::EarlyDiffuseModAmount]=0.4838999807834625;
        FXSlowBraaam[Parameter::EarlyDiffuseModRate]=0.3026999831199646;
        FXSlowBraaam[Parameter::EarlyOut]=0.0;
        FXSlowBraaam[Parameter::EqCrossSeed]=0.1759999990463257;
        FXSlowBraaam[Parameter::EqCutoff]=0.7439999580383301;
        FXSlowBraaam[Parameter::EqHighFreq]=0.5813999772071838;
        FXSlowBraaam[Parameter::EqHighGain]=0.8479999899864197;
        FXSlowBraaam[Parameter::EqHighShelfEnabled]=1.0;
        FXSlowBraaam[Parameter::EqLowFreq]=0.1119999960064888;
        FXSlowBraaam[Parameter::EqLowGain]=0.8240000009536743;
        FXSlowBraaam[Parameter::EqLowShelfEnabled]=0.0;
        FXSlowBraaam[Parameter::EqLowpassEnabled]=0.0;
        FXSlowBraaam[Parameter::LowCut]=0.3599999845027924;
        FXSlowBraaam[Parameter::LowCutEnabled]=1.0;
        FXSlowBraaam[Parameter::InputMix]=0.1319999992847443;
        FXSlowBraaam[Parameter::Interpolation]=0.0;
        FXSlowBraaam[Parameter::LateDiffuseCount]=0.4879999756813049;
        FXSlowBraaam[Parameter::LateDiffuseDelay]=0.363999992609024;
        FXSlowBraaam[Parameter::LateDiffuseEnabled]=1.0;
        FXSlowBraaam[Parameter::LateDiffuseFeedback]=0.7106999754905701;
        FXSlowBraaam[Parameter::LateDiffuseModAmount]=0.4387999773025513;
        FXSlowBraaam[Parameter::LateDiffuseModRate]=0.1826999932527542;
        FXSlowBraaam[Parameter::LateLineCount]=1.0;
        FXSlowBraaam[Parameter::LateLineDecay]=0.5640000104904175;
        FXSlowBraaam[Parameter::LateLineModAmount]=0.1519999951124191;
        FXSlowBraaam[Parameter::LateLineModRate]=0.2532999813556671;
        FXSlowBraaam[Parameter::LateLineSize]=0.08399999886751175;
        FXSlowBraaam[Parameter::LateMode]=1.0;
        FXSlowBraaam[Parameter::LateOut]=0.6839999556541443;
        FXSlowBraaam[Parameter::HighCut]=0.1400000005960464;
        FXSlowBraaam[Parameter::HighCutEnabled]=1.0;
        FXSlowBraaam[Parameter::SeedDelay]=0.1730999946594238;
        FXSlowBraaam[Parameter::SeedDiffusion]=0.3240000009536743;
        FXSlowBraaam[Parameter::SeedPostDiffusion]=0.3233000040054321;
        FXSlowBraaam[Parameter::SeedTap]=0.7879999876022339;
        FXSlowBraaam[Parameter::TapDecay]=0.0;
        FXSlowBraaam[Parameter::TapLength]=1.0;
        FXSlowBraaam[Parameter::TapPredelay]=0.0;
        FXSlowBraaam[Parameter::TapCount]=0.5839999914169312;
        FXSlowBraaam[Parameter::TapEnabled]=1.0;

        FXTheUpsideDown[Parameter::DryOut]=0.7080000042915344;
        FXTheUpsideDown[Parameter::EarlyDiffuseCount]=1.0;
        FXTheUpsideDown[Parameter::EarlyDiffuseDelay]=0.9466999769210815;
        FXTheUpsideDown[Parameter::EarlyDiffuseEnabled]=1.0;
        FXTheUpsideDown[Parameter::EarlyDiffuseFeedback]=0.510699987411499;
        FXTheUpsideDown[Parameter::EarlyDiffuseModAmount]=0.5598999857902527;
        FXTheUpsideDown[Parameter::EarlyDiffuseModRate]=0.4587000012397766;
        FXTheUpsideDown[Parameter::EarlyOut]=0.9559999704360962;
        FXTheUpsideDown[Parameter::EqCrossSeed]=0.1319999992847443;
        FXTheUpsideDown[Parameter::EqCutoff]=0.7439999580383301;
        FXTheUpsideDown[Parameter::EqHighFreq]=0.5133999586105347;
        FXTheUpsideDown[Parameter::EqHighGain]=0.7680000066757202;
        FXTheUpsideDown[Parameter::EqHighShelfEnabled]=0.0;
        FXTheUpsideDown[Parameter::EqLowFreq]=0.4079999923706055;
        FXTheUpsideDown[Parameter::EqLowGain]=0.5559999942779541;
        FXTheUpsideDown[Parameter::EqLowShelfEnabled]=0.0;
        FXTheUpsideDown[Parameter::EqLowpassEnabled]=0.0;
        FXTheUpsideDown[Parameter::LowCut]=0.1772999912500381;
        FXTheUpsideDown[Parameter::LowCutEnabled]=0.0;
        FXTheUpsideDown[Parameter::InputMix]=0.2800000011920929;
        FXTheUpsideDown[Parameter::Interpolation]=1.0;
        FXTheUpsideDown[Parameter::LateDiffuseCount]=0.4879999756813049;
        FXTheUpsideDown[Parameter::LateDiffuseDelay]=0.239999994635582;
        FXTheUpsideDown[Parameter::LateDiffuseEnabled]=0.0;
        FXTheUpsideDown[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        FXTheUpsideDown[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        FXTheUpsideDown[Parameter::LateDiffuseModRate]=0.1666999906301498;
        FXTheUpsideDown[Parameter::LateLineCount]=0.6039999723434448;
        FXTheUpsideDown[Parameter::LateLineDecay]=0.715999960899353;
        FXTheUpsideDown[Parameter::LateLineModAmount]=0.3319999873638153;
        FXTheUpsideDown[Parameter::LateLineModRate]=0.1412999927997589;
        FXTheUpsideDown[Parameter::LateLineSize]=0.3919999897480011;
        FXTheUpsideDown[Parameter::LateMode]=1.0;
        FXTheUpsideDown[Parameter::LateOut]=0.6279999613761902;
        FXTheUpsideDown[Parameter::HighCut]=0.3759999871253967;
        FXTheUpsideDown[Parameter::HighCutEnabled]=1.0;
        FXTheUpsideDown[Parameter::SeedDelay]=0.3800999820232391;
        FXTheUpsideDown[Parameter::SeedDiffusion]=0.1129999980330467;
        FXTheUpsideDown[Parameter::SeedPostDiffusion]=0.2633000016212463;
        FXTheUpsideDown[Parameter::SeedTap]=0.8539999723434448;
        FXTheUpsideDown[Parameter::TapDecay]=0.0;
        FXTheUpsideDown[Parameter::TapLength]=1.0;
        FXTheUpsideDown[Parameter::TapPredelay]=0.0;
        FXTheUpsideDown[Parameter::TapCount]=0.1599999964237213;
        FXTheUpsideDown[Parameter::TapEnabled]=1.0;

        LBigSoundStage[Parameter::DryOut]=1.0;
        LBigSoundStage[Parameter::EarlyDiffuseCount]=0.4172999858856201;
        LBigSoundStage[Parameter::EarlyDiffuseDelay]=0.6947000026702881;
        LBigSoundStage[Parameter::EarlyDiffuseEnabled]=1.0;
        LBigSoundStage[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        LBigSoundStage[Parameter::EarlyDiffuseModAmount]=0.3425999879837036;
        LBigSoundStage[Parameter::EarlyDiffuseModRate]=0.3506999909877777;
        LBigSoundStage[Parameter::EarlyOut]=0.687999963760376;
        LBigSoundStage[Parameter::EqCrossSeed]=0.1119999960064888;
        LBigSoundStage[Parameter::EqCutoff]=0.8172999620437622;
        LBigSoundStage[Parameter::EqHighFreq]=0.5346999764442444;
        LBigSoundStage[Parameter::EqHighGain]=0.543999969959259;
        LBigSoundStage[Parameter::EqHighShelfEnabled]=0.0;
        LBigSoundStage[Parameter::EqLowFreq]=0.2639999985694885;
        LBigSoundStage[Parameter::EqLowGain]=0.9720000028610229;
        LBigSoundStage[Parameter::EqLowShelfEnabled]=1.0;
        LBigSoundStage[Parameter::EqLowpassEnabled]=0.0;
        LBigSoundStage[Parameter::LowCut]=0.2933000028133392;
        LBigSoundStage[Parameter::LowCutEnabled]=0.0;
        LBigSoundStage[Parameter::InputMix]=0.2346999943256378;
        LBigSoundStage[Parameter::Interpolation]=0.0;
        LBigSoundStage[Parameter::LateDiffuseCount]=1.0;
        LBigSoundStage[Parameter::LateDiffuseDelay]=0.1759999990463257;
        LBigSoundStage[Parameter::LateDiffuseEnabled]=1.0;
        LBigSoundStage[Parameter::LateDiffuseFeedback]=0.8840000033378601;
        LBigSoundStage[Parameter::LateDiffuseModAmount]=0.1959999948740005;
        LBigSoundStage[Parameter::LateDiffuseModRate]=0.2586999833583832;
        LBigSoundStage[Parameter::LateLineCount]=0.6240000128746033;
        LBigSoundStage[Parameter::LateLineDecay]=0.4120000004768372;
        LBigSoundStage[Parameter::LateLineModAmount]=0.0;
        LBigSoundStage[Parameter::LateLineModRate]=0.0;
        LBigSoundStage[Parameter::LateLineSize]=0.2479999959468842;
        LBigSoundStage[Parameter::LateMode]=1.0;
        LBigSoundStage[Parameter::LateOut]=0.7559999823570251;
        LBigSoundStage[Parameter::HighCut]=1.0;
        LBigSoundStage[Parameter::HighCutEnabled]=0.0;
        LBigSoundStage[Parameter::SeedDelay]=0.2309999912977219;
        LBigSoundStage[Parameter::SeedDiffusion]=0.1469999998807907;
        LBigSoundStage[Parameter::SeedPostDiffusion]=0.2899999916553497;
        LBigSoundStage[Parameter::SeedTap]=0.3339999914169312;
        LBigSoundStage[Parameter::TapDecay]=1.0;
        LBigSoundStage[Parameter::TapLength]=0.9866999983787537;
        LBigSoundStage[Parameter::TapPredelay]=0.1599999964237213;
        LBigSoundStage[Parameter::TapCount]=0.1959999948740005;
        LBigSoundStage[Parameter::TapEnabled]=0.0;

        LDiffusionCyclone[Parameter::DryOut]=0.9079999923706055;
        LDiffusionCyclone[Parameter::EarlyDiffuseCount]=0.2132999897003174;
        LDiffusionCyclone[Parameter::EarlyDiffuseDelay]=0.2906999886035919;
        LDiffusionCyclone[Parameter::EarlyDiffuseEnabled]=1.0;
        LDiffusionCyclone[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        LDiffusionCyclone[Parameter::EarlyDiffuseModAmount]=0.1145999953150749;
        LDiffusionCyclone[Parameter::EarlyDiffuseModRate]=0.2226999998092651;
        LDiffusionCyclone[Parameter::EarlyOut]=0.0;
        LDiffusionCyclone[Parameter::EqCrossSeed]=0.1119999960064888;
        LDiffusionCyclone[Parameter::EqCutoff]=0.8172999620437622;
        LDiffusionCyclone[Parameter::EqHighFreq]=0.5346999764442444;
        LDiffusionCyclone[Parameter::EqHighGain]=0.543999969959259;
        LDiffusionCyclone[Parameter::EqHighShelfEnabled]=0.0;
        LDiffusionCyclone[Parameter::EqLowFreq]=0.3879999816417694;
        LDiffusionCyclone[Parameter::EqLowGain]=0.5559999942779541;
        LDiffusionCyclone[Parameter::EqLowShelfEnabled]=0.0;
        LDiffusionCyclone[Parameter::EqLowpassEnabled]=1.0;
        LDiffusionCyclone[Parameter::LowCut]=0.1292999982833862;
        LDiffusionCyclone[Parameter::LowCutEnabled]=0.0;
        LDiffusionCyclone[Parameter::InputMix]=0.0746999979019165;
        LDiffusionCyclone[Parameter::Interpolation]=0.0;
        LDiffusionCyclone[Parameter::LateDiffuseCount]=0.4786999821662903;
        LDiffusionCyclone[Parameter::LateDiffuseDelay]=0.3759999871253967;
        LDiffusionCyclone[Parameter::LateDiffuseEnabled]=1.0;
        LDiffusionCyclone[Parameter::LateDiffuseFeedback]=0.7080000042915344;
        LDiffusionCyclone[Parameter::LateDiffuseModAmount]=0.2893999814987183;
        LDiffusionCyclone[Parameter::LateDiffuseModRate]=0.2506999969482422;
        LDiffusionCyclone[Parameter::LateLineCount]=1.0;
        LDiffusionCyclone[Parameter::LateLineDecay]=0.7519999742507935;
        LDiffusionCyclone[Parameter::LateLineModAmount]=0.2586999833583832;
        LDiffusionCyclone[Parameter::LateLineModRate]=0.2239999920129776;
        LDiffusionCyclone[Parameter::LateLineSize]=0.3199999928474426;
        LDiffusionCyclone[Parameter::LateMode]=1.0;
        LDiffusionCyclone[Parameter::LateOut]=0.7786999940872192;
        LDiffusionCyclone[Parameter::HighCut]=0.6119999885559082;
        LDiffusionCyclone[Parameter::HighCutEnabled]=0.0;
        LDiffusionCyclone[Parameter::SeedDelay]=0.3039999902248383;
        LDiffusionCyclone[Parameter::SeedDiffusion]=0.1219999939203262;
        LDiffusionCyclone[Parameter::SeedPostDiffusion]=0.3409999907016754;
        LDiffusionCyclone[Parameter::SeedTap]=0.0;
        LDiffusionCyclone[Parameter::TapDecay]=1.0;
        LDiffusionCyclone[Parameter::TapLength]=0.9866999983787537;
        LDiffusionCyclone[Parameter::TapPredelay]=0.1599999964237213;
        LDiffusionCyclone[Parameter::TapCount]=0.1959999948740005;
        LDiffusionCyclone[Parameter::TapEnabled]=0.0;

        LScreamIntoTheVoid[Parameter::DryOut]=0.6998999714851379;
        LScreamIntoTheVoid[Parameter::EarlyDiffuseCount]=0.7172999978065491;
        LScreamIntoTheVoid[Parameter::EarlyDiffuseDelay]=0.7386999726295471;
        LScreamIntoTheVoid[Parameter::EarlyDiffuseEnabled]=1.0;
        LScreamIntoTheVoid[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        LScreamIntoTheVoid[Parameter::EarlyDiffuseModAmount]=0.2505999803543091;
        LScreamIntoTheVoid[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        LScreamIntoTheVoid[Parameter::EarlyOut]=0.0;
        LScreamIntoTheVoid[Parameter::EqCrossSeed]=0.1119999960064888;
        LScreamIntoTheVoid[Parameter::EqCutoff]=0.5012999773025513;
        LScreamIntoTheVoid[Parameter::EqHighFreq]=0.5346999764442444;
        LScreamIntoTheVoid[Parameter::EqHighGain]=0.543999969959259;
        LScreamIntoTheVoid[Parameter::EqHighShelfEnabled]=1.0;
        LScreamIntoTheVoid[Parameter::EqLowFreq]=0.3879999816417694;
        LScreamIntoTheVoid[Parameter::EqLowGain]=0.5559999942779541;
        LScreamIntoTheVoid[Parameter::EqLowShelfEnabled]=0.0;
        LScreamIntoTheVoid[Parameter::EqLowpassEnabled]=0.0;
        LScreamIntoTheVoid[Parameter::LowCut]=0.2933000028133392;
        LScreamIntoTheVoid[Parameter::LowCutEnabled]=1.0;
        LScreamIntoTheVoid[Parameter::InputMix]=0.2346999943256378;
        LScreamIntoTheVoid[Parameter::Interpolation]=0.0;
        LScreamIntoTheVoid[Parameter::LateDiffuseCount]=0.4186999797821045;
        LScreamIntoTheVoid[Parameter::LateDiffuseDelay]=0.3519999980926514;
        LScreamIntoTheVoid[Parameter::LateDiffuseEnabled]=1.0;
        LScreamIntoTheVoid[Parameter::LateDiffuseFeedback]=0.7119999527931213;
        LScreamIntoTheVoid[Parameter::LateDiffuseModAmount]=0.3653999865055084;
        LScreamIntoTheVoid[Parameter::LateDiffuseModRate]=0.1666999906301498;
        LScreamIntoTheVoid[Parameter::LateLineCount]=0.8319999575614929;
        LScreamIntoTheVoid[Parameter::LateLineDecay]=0.7199999690055847;
        LScreamIntoTheVoid[Parameter::LateLineModAmount]=0.146699994802475;
        LScreamIntoTheVoid[Parameter::LateLineModRate]=0.1959999948740005;
        LScreamIntoTheVoid[Parameter::LateLineSize]=0.3999999761581421;
        LScreamIntoTheVoid[Parameter::LateMode]=1.0;
        LScreamIntoTheVoid[Parameter::LateOut]=0.7786999940872192;
        LScreamIntoTheVoid[Parameter::HighCut]=0.4852999746799469;
        LScreamIntoTheVoid[Parameter::HighCutEnabled]=0.0;
        LScreamIntoTheVoid[Parameter::SeedDelay]=0.1939999908208847;
        LScreamIntoTheVoid[Parameter::SeedDiffusion]=0.07299999892711639;
        LScreamIntoTheVoid[Parameter::SeedPostDiffusion]=0.3039999902248383;
        LScreamIntoTheVoid[Parameter::SeedTap]=0.3339999914169312;
        LScreamIntoTheVoid[Parameter::TapDecay]=1.0;
        LScreamIntoTheVoid[Parameter::TapLength]=0.9866999983787537;
        LScreamIntoTheVoid[Parameter::TapPredelay]=0.1599999964237213;
        LScreamIntoTheVoid[Parameter::TapCount]=0.1959999948740005;
        LScreamIntoTheVoid[Parameter::TapEnabled]=0.0;

        M90sDigitalReverb[Parameter::DryOut]=0.9720000028610229;
        M90sDigitalReverb[Parameter::EarlyDiffuseCount]=0.2960000038146973;
        M90sDigitalReverb[Parameter::EarlyDiffuseDelay]=0.5467000007629395;
        M90sDigitalReverb[Parameter::EarlyDiffuseEnabled]=1.0;
        M90sDigitalReverb[Parameter::EarlyDiffuseFeedback]=0.5787000060081482;
        M90sDigitalReverb[Parameter::EarlyDiffuseModAmount]=0.3039000034332275;
        M90sDigitalReverb[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        M90sDigitalReverb[Parameter::EarlyOut]=0.4959999918937683;
        M90sDigitalReverb[Parameter::EqCrossSeed]=0.0;
        M90sDigitalReverb[Parameter::EqCutoff]=0.7439999580383301;
        M90sDigitalReverb[Parameter::EqHighFreq]=0.5133999586105347;
        M90sDigitalReverb[Parameter::EqHighGain]=0.7680000066757202;
        M90sDigitalReverb[Parameter::EqHighShelfEnabled]=0.0;
        M90sDigitalReverb[Parameter::EqLowFreq]=0.4079999923706055;
        M90sDigitalReverb[Parameter::EqLowGain]=0.5559999942779541;
        M90sDigitalReverb[Parameter::EqLowShelfEnabled]=0.0;
        M90sDigitalReverb[Parameter::EqLowpassEnabled]=1.0;
        M90sDigitalReverb[Parameter::LowCut]=0.2933000028133392;
        M90sDigitalReverb[Parameter::LowCutEnabled]=0.0;
        M90sDigitalReverb[Parameter::InputMix]=0.2800000011920929;
        M90sDigitalReverb[Parameter::Interpolation]=1.0;
        M90sDigitalReverb[Parameter::LateDiffuseCount]=0.4879999756813049;
        M90sDigitalReverb[Parameter::LateDiffuseDelay]=0.239999994635582;
        M90sDigitalReverb[Parameter::LateDiffuseEnabled]=1.0;
        M90sDigitalReverb[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        M90sDigitalReverb[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        M90sDigitalReverb[Parameter::LateDiffuseModRate]=0.1666999906301498;
        M90sDigitalReverb[Parameter::LateLineCount]=1.0;
        M90sDigitalReverb[Parameter::LateLineDecay]=0.3120000064373016;
        M90sDigitalReverb[Parameter::LateLineModAmount]=0.2719999849796295;
        M90sDigitalReverb[Parameter::LateLineModRate]=0.0372999981045723;
        M90sDigitalReverb[Parameter::LateLineSize]=0.2680000066757202;
        M90sDigitalReverb[Parameter::LateMode]=1.0;
        M90sDigitalReverb[Parameter::LateOut]=0.6173999905586243;
        M90sDigitalReverb[Parameter::HighCut]=0.7239999771118164;
        M90sDigitalReverb[Parameter::HighCutEnabled]=1.0;
        M90sDigitalReverb[Parameter::SeedDelay]=0.2180999964475632;
        M90sDigitalReverb[Parameter::SeedDiffusion]=0.1850000023841858;
        M90sDigitalReverb[Parameter::SeedPostDiffusion]=0.3652999997138977;
        M90sDigitalReverb[Parameter::SeedTap]=0.3339999914169312;
        M90sDigitalReverb[Parameter::TapDecay]=1.0;
        M90sDigitalReverb[Parameter::TapLength]=0.9866999983787537;
        M90sDigitalReverb[Parameter::TapPredelay]=0.0;
        M90sDigitalReverb[Parameter::TapCount]=0.1959999948740005;
        M90sDigitalReverb[Parameter::TapEnabled]=0.0;

        MAiryAmbience[Parameter::DryOut]=0.9785999655723572;
        MAiryAmbience[Parameter::EarlyDiffuseCount]=0.2360000014305115;
        MAiryAmbience[Parameter::EarlyDiffuseDelay]=0.3066999912261963;
        MAiryAmbience[Parameter::EarlyDiffuseEnabled]=1.0;
        MAiryAmbience[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        MAiryAmbience[Parameter::EarlyDiffuseModAmount]=0.143899992108345;
        MAiryAmbience[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        MAiryAmbience[Parameter::EarlyOut]=0.5559999942779541;
        MAiryAmbience[Parameter::EqCrossSeed]=0.0;
        MAiryAmbience[Parameter::EqCutoff]=0.9759999513626099;
        MAiryAmbience[Parameter::EqHighFreq]=0.7013999819755554;
        MAiryAmbience[Parameter::EqHighGain]=0.2746999859809875;
        MAiryAmbience[Parameter::EqHighShelfEnabled]=1.0;
        MAiryAmbience[Parameter::EqLowFreq]=0.1959999948740005;
        MAiryAmbience[Parameter::EqLowGain]=0.687999963760376;
        MAiryAmbience[Parameter::EqLowShelfEnabled]=1.0;
        MAiryAmbience[Parameter::EqLowpassEnabled]=0.0;
        MAiryAmbience[Parameter::LowCut]=0.0;
        MAiryAmbience[Parameter::LowCutEnabled]=0.0;
        MAiryAmbience[Parameter::InputMix]=0.07069999724626541;
        MAiryAmbience[Parameter::Interpolation]=1.0;
        MAiryAmbience[Parameter::LateDiffuseCount]=0.4186999797821045;
        MAiryAmbience[Parameter::LateDiffuseDelay]=0.2506999969482422;
        MAiryAmbience[Parameter::LateDiffuseEnabled]=0.0;
        MAiryAmbience[Parameter::LateDiffuseFeedback]=0.7119999527931213;
        MAiryAmbience[Parameter::LateDiffuseModAmount]=0.2480999976396561;
        MAiryAmbience[Parameter::LateDiffuseModRate]=0.1666999906301498;
        MAiryAmbience[Parameter::LateLineCount]=0.8319999575614929;
        MAiryAmbience[Parameter::LateLineDecay]=0.5879999995231628;
        MAiryAmbience[Parameter::LateLineModAmount]=0.2719999849796295;
        MAiryAmbience[Parameter::LateLineModRate]=0.2292999923229218;
        MAiryAmbience[Parameter::LateLineSize]=0.1079999953508377;
        MAiryAmbience[Parameter::LateMode]=1.0;
        MAiryAmbience[Parameter::LateOut]=0.4280999898910522;
        MAiryAmbience[Parameter::HighCut]=1.0;
        MAiryAmbience[Parameter::HighCutEnabled]=0.0;
        MAiryAmbience[Parameter::SeedDelay]=0.2180999964475632;
        MAiryAmbience[Parameter::SeedDiffusion]=0.1850000023841858;
        MAiryAmbience[Parameter::SeedPostDiffusion]=0.3652999997138977;
        MAiryAmbience[Parameter::SeedTap]=0.3339999914169312;
        MAiryAmbience[Parameter::TapDecay]=1.0;
        MAiryAmbience[Parameter::TapLength]=0.9866999983787537;
        MAiryAmbience[Parameter::TapPredelay]=0.0;
        MAiryAmbience[Parameter::TapCount]=0.1959999948740005;
        MAiryAmbience[Parameter::TapEnabled]=0.0;

        MDarkPlate[Parameter::DryOut]=0.8705999851226807;
        MDarkPlate[Parameter::EarlyDiffuseCount]=0.2960000038146973;
        MDarkPlate[Parameter::EarlyDiffuseDelay]=0.3066999912261963;
        MDarkPlate[Parameter::EarlyDiffuseEnabled]=0.0;
        MDarkPlate[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        MDarkPlate[Parameter::EarlyDiffuseModAmount]=0.143899992108345;
        MDarkPlate[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        MDarkPlate[Parameter::EarlyOut]=0.0;
        MDarkPlate[Parameter::EqCrossSeed]=0.0;
        MDarkPlate[Parameter::EqCutoff]=0.9759999513626099;
        MDarkPlate[Parameter::EqHighFreq]=0.5133999586105347;
        MDarkPlate[Parameter::EqHighGain]=0.7680000066757202;
        MDarkPlate[Parameter::EqHighShelfEnabled]=1.0;
        MDarkPlate[Parameter::EqLowFreq]=0.3879999816417694;
        MDarkPlate[Parameter::EqLowGain]=0.5559999942779541;
        MDarkPlate[Parameter::EqLowShelfEnabled]=0.0;
        MDarkPlate[Parameter::EqLowpassEnabled]=0.0;
        MDarkPlate[Parameter::HighCut]=0.2933000028133392;
        MDarkPlate[Parameter::HighCutEnabled]=0.0;
        MDarkPlate[Parameter::InputMix]=0.2346999943256378;
        MDarkPlate[Parameter::Interpolation]=1.0;
        MDarkPlate[Parameter::LateDiffuseCount]=0.4879999756813049;
        MDarkPlate[Parameter::LateDiffuseDelay]=0.239999994635582;
        MDarkPlate[Parameter::LateDiffuseEnabled]=1.0;
        MDarkPlate[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        MDarkPlate[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        MDarkPlate[Parameter::LateDiffuseModRate]=0.1666999906301498;
        MDarkPlate[Parameter::LateLineCount]=1.0;
        MDarkPlate[Parameter::LateLineDecay]=0.6345999836921692;
        MDarkPlate[Parameter::LateLineModAmount]=0.2719999849796295;
        MDarkPlate[Parameter::LateLineModRate]=0.2292999923229218;
        MDarkPlate[Parameter::LateLineSize]=0.4693999886512756;
        MDarkPlate[Parameter::LateMode]=1.0;
        MDarkPlate[Parameter::LateOut]=0.6613999605178833;
        MDarkPlate[Parameter::LowCut]=0.6399999856948853;
        MDarkPlate[Parameter::LowCutEnabled]=1.0;
        MDarkPlate[Parameter::SeedDelay]=0.2180999964475632;
        MDarkPlate[Parameter::SeedDiffusion]=0.1850000023841858;
        MDarkPlate[Parameter::SeedPostDiffusion]=0.3652999997138977;
        MDarkPlate[Parameter::SeedTap]=0.3339999914169312;
        MDarkPlate[Parameter::TapDecay]=1.0;
        MDarkPlate[Parameter::TapLength]=0.9866999983787537;
        MDarkPlate[Parameter::TapPredelay]=0.0;
        MDarkPlate[Parameter::TapCount]=0.1959999948740005;
        MDarkPlate[Parameter::TapEnabled]=0.0;

        MGhostly[Parameter::DryOut]=1.0;
        MGhostly[Parameter::EarlyDiffuseCount]=0.1560000032186508;
        MGhostly[Parameter::EarlyDiffuseDelay]=0.5986999869346619;
        MGhostly[Parameter::EarlyDiffuseEnabled]=1.0;
        MGhostly[Parameter::EarlyDiffuseFeedback]=0.6987000107765198;
        MGhostly[Parameter::EarlyDiffuseModAmount]=0.927899956703186;
        MGhostly[Parameter::EarlyDiffuseModRate]=0.2866999804973602;
        MGhostly[Parameter::EarlyOut]=0.7199999690055847;
        MGhostly[Parameter::EqCrossSeed]=0.1759999990463257;
        MGhostly[Parameter::EqCutoff]=0.7439999580383301;
        MGhostly[Parameter::EqHighFreq]=0.6413999795913696;
        MGhostly[Parameter::EqHighGain]=0.8319999575614929;
        MGhostly[Parameter::EqHighShelfEnabled]=1.0;
        MGhostly[Parameter::EqLowFreq]=0.1119999960064888;
        MGhostly[Parameter::EqLowGain]=0.8240000009536743;
        MGhostly[Parameter::EqLowShelfEnabled]=1.0;
        MGhostly[Parameter::EqLowpassEnabled]=0.0;
        MGhostly[Parameter::LowCut]=0.0;
        MGhostly[Parameter::LowCutEnabled]=0.0;
        MGhostly[Parameter::InputMix]=0.0;
        MGhostly[Parameter::Interpolation]=0.0;
        MGhostly[Parameter::LateDiffuseCount]=0.4879999756813049;
        MGhostly[Parameter::LateDiffuseDelay]=0.239999994635582;
        MGhostly[Parameter::LateDiffuseEnabled]=0.0;
        MGhostly[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        MGhostly[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        MGhostly[Parameter::LateDiffuseModRate]=0.1666999906301498;
        MGhostly[Parameter::LateLineCount]=1.0;
        MGhostly[Parameter::LateLineDecay]=0.5640000104904175;
        MGhostly[Parameter::LateLineModAmount]=0.335999995470047;
        MGhostly[Parameter::LateLineModRate]=0.2532999813556671;
        MGhostly[Parameter::LateLineSize]=0.08399999886751175;
        MGhostly[Parameter::LateMode]=1.0;
        MGhostly[Parameter::LateOut]=0.4799999892711639;
        MGhostly[Parameter::HighCut]=0.1920000016689301;
        MGhostly[Parameter::HighCutEnabled]=1.0;
        MGhostly[Parameter::SeedDelay]=0.1730999946594238;
        MGhostly[Parameter::SeedDiffusion]=0.3240000009536743;
        MGhostly[Parameter::SeedPostDiffusion]=0.3233000040054321;
        MGhostly[Parameter::SeedTap]=0.7879999876022339;
        MGhostly[Parameter::TapDecay]=1.0;
        MGhostly[Parameter::TapLength]=0.7080000042915344;
        MGhostly[Parameter::TapPredelay]=0.0;
        MGhostly[Parameter::TapCount]=0.03599999845027924;
        MGhostly[Parameter::TapEnabled]=1.0;

        MTappedLines[Parameter::DryOut]=0.9720000028610229;
        MTappedLines[Parameter::EarlyDiffuseCount]=0.4839999973773956;
        MTappedLines[Parameter::EarlyDiffuseDelay]=0.2307000011205673;
        MTappedLines[Parameter::EarlyDiffuseEnabled]=1.0;
        MTappedLines[Parameter::EarlyDiffuseFeedback]=0.7426999807357788;
        MTappedLines[Parameter::EarlyDiffuseModAmount]=0.6678999662399292;
        MTappedLines[Parameter::EarlyDiffuseModRate]=0.2866999804973602;
        MTappedLines[Parameter::EarlyOut]=1.0;
        MTappedLines[Parameter::EqCrossSeed]=0.6599999666213989;
        MTappedLines[Parameter::EqCutoff]=0.7439999580383301;
        MTappedLines[Parameter::EqHighFreq]=0.5133999586105347;
        MTappedLines[Parameter::EqHighGain]=0.7680000066757202;
        MTappedLines[Parameter::EqHighShelfEnabled]=0.0;
        MTappedLines[Parameter::EqLowFreq]=0.4079999923706055;
        MTappedLines[Parameter::EqLowGain]=0.5559999942779541;
        MTappedLines[Parameter::EqLowShelfEnabled]=0.0;
        MTappedLines[Parameter::EqLowpassEnabled]=1.0;
        MTappedLines[Parameter::LowCut]=0.6092999577522278;
        MTappedLines[Parameter::LowCutEnabled]=1.0;
        MTappedLines[Parameter::InputMix]=0.0;
        MTappedLines[Parameter::Interpolation]=1.0;
        MTappedLines[Parameter::LateDiffuseCount]=0.4879999756813049;
        MTappedLines[Parameter::LateDiffuseDelay]=0.239999994635582;
        MTappedLines[Parameter::LateDiffuseEnabled]=0.0;
        MTappedLines[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        MTappedLines[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        MTappedLines[Parameter::LateDiffuseModRate]=0.1666999906301498;
        MTappedLines[Parameter::LateLineCount]=0.515999972820282;
        MTappedLines[Parameter::LateLineDecay]=0.715999960899353;
        MTappedLines[Parameter::LateLineModAmount]=0.171999990940094;
        MTappedLines[Parameter::LateLineModRate]=0.2572999894618988;
        MTappedLines[Parameter::LateLineSize]=0.2680000066757202;
        MTappedLines[Parameter::LateMode]=1.0;
        MTappedLines[Parameter::LateOut]=0.5679999589920044;
        MTappedLines[Parameter::HighCut]=0.7239999771118164;
        MTappedLines[Parameter::HighCutEnabled]=0.0;
        MTappedLines[Parameter::SeedDelay]=0.09409999847412109;
        MTappedLines[Parameter::SeedDiffusion]=0.3240000009536743;
        MTappedLines[Parameter::SeedPostDiffusion]=0.3233000040054321;
        MTappedLines[Parameter::SeedTap]=0.4639999866485596;
        MTappedLines[Parameter::TapDecay]=0.7719999551773071;
        MTappedLines[Parameter::TapLength]=0.8586999773979187;
        MTappedLines[Parameter::TapPredelay]=0.0;
        MTappedLines[Parameter::TapCount]=0.06399999558925629;
        MTappedLines[Parameter::TapEnabled]=1.0;

        SFastAttack[Parameter::DryOut]=0.9720000028610229;
        SFastAttack[Parameter::EarlyDiffuseCount]=0.2960000038146973;
        SFastAttack[Parameter::EarlyDiffuseDelay]=0.5467000007629395;
        SFastAttack[Parameter::EarlyDiffuseEnabled]=1.0;
        SFastAttack[Parameter::EarlyDiffuseFeedback]=0.5787000060081482;
        SFastAttack[Parameter::EarlyDiffuseModAmount]=0.3039000034332275;
        SFastAttack[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        SFastAttack[Parameter::EarlyOut]=0.6800000071525574;
        SFastAttack[Parameter::EqCrossSeed]=0.2680000066757202;
        SFastAttack[Parameter::EqCutoff]=0.7439999580383301;
        SFastAttack[Parameter::EqHighFreq]=0.5133999586105347;
        SFastAttack[Parameter::EqHighGain]=0.7680000066757202;
        SFastAttack[Parameter::EqHighShelfEnabled]=0.0;
        SFastAttack[Parameter::EqLowFreq]=0.4079999923706055;
        SFastAttack[Parameter::EqLowGain]=0.5559999942779541;
        SFastAttack[Parameter::EqLowShelfEnabled]=0.0;
        SFastAttack[Parameter::EqLowpassEnabled]=1.0;
        SFastAttack[Parameter::LowCut]=0.2933000028133392;
        SFastAttack[Parameter::LowCutEnabled]=0.0;
        SFastAttack[Parameter::InputMix]=0.2800000011920929;
        SFastAttack[Parameter::Interpolation]=1.0;
        SFastAttack[Parameter::LateDiffuseCount]=0.4879999756813049;
        SFastAttack[Parameter::LateDiffuseDelay]=0.239999994635582;
        SFastAttack[Parameter::LateDiffuseEnabled]=0.0;
        SFastAttack[Parameter::LateDiffuseFeedback]=0.8506999611854553;
        SFastAttack[Parameter::LateDiffuseModAmount]=0.1467999964952469;
        SFastAttack[Parameter::LateDiffuseModRate]=0.1666999906301498;
        SFastAttack[Parameter::LateLineCount]=0.3079999983310699;
        SFastAttack[Parameter::LateLineDecay]=0.4439999759197235;
        SFastAttack[Parameter::LateLineModAmount]=0.2719999849796295;
        SFastAttack[Parameter::LateLineModRate]=0.0372999981045723;
        SFastAttack[Parameter::LateLineSize]=0.4079999923706055;
        SFastAttack[Parameter::LateMode]=1.0;
        SFastAttack[Parameter::LateOut]=0.5399999618530273;
        SFastAttack[Parameter::HighCut]=0.7239999771118164;
        SFastAttack[Parameter::HighCutEnabled]=1.0;
        SFastAttack[Parameter::SeedDelay]=0.2180999964475632;
        SFastAttack[Parameter::SeedDiffusion]=0.1850000023841858;
        SFastAttack[Parameter::SeedPostDiffusion]=0.3652999997138977;
        SFastAttack[Parameter::SeedTap]=0.3489999771118164;
        SFastAttack[Parameter::TapDecay]=0.9559999704360962;
        SFastAttack[Parameter::TapLength]=0.6520000100135803;
        SFastAttack[Parameter::TapPredelay]=0.0;
        SFastAttack[Parameter::TapCount]=0.06799999624490738;
        SFastAttack[Parameter::TapEnabled]=1.0;

        SSmallPlate[Parameter::DryOut]=0.8705999851226807;
        SSmallPlate[Parameter::EarlyDiffuseCount]=0.2960000038146973;
        SSmallPlate[Parameter::EarlyDiffuseDelay]=0.3066999912261963;
        SSmallPlate[Parameter::EarlyDiffuseEnabled]=1.0;
        SSmallPlate[Parameter::EarlyDiffuseFeedback]=0.7706999778747559;
        SSmallPlate[Parameter::EarlyDiffuseModAmount]=0.143899992108345;
        SSmallPlate[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        SSmallPlate[Parameter::EarlyOut]=0.3680000007152557;
        SSmallPlate[Parameter::EqCrossSeed]=0.0;
        SSmallPlate[Parameter::EqCutoff]=0.9759999513626099;
        SSmallPlate[Parameter::EqHighFreq]=0.7053999900817871;
        SSmallPlate[Parameter::EqHighGain]=0.8906999826431274;
        SSmallPlate[Parameter::EqHighShelfEnabled]=1.0;
        SSmallPlate[Parameter::EqLowFreq]=0.3879999816417694;
        SSmallPlate[Parameter::EqLowGain]=0.5559999942779541;
        SSmallPlate[Parameter::EqLowShelfEnabled]=0.0;
        SSmallPlate[Parameter::EqLowpassEnabled]=0.0;
        SSmallPlate[Parameter::LowCut]=0.2933000028133392;
        SSmallPlate[Parameter::LowCutEnabled]=0.0;
        SSmallPlate[Parameter::InputMix]=0.2346999943256378;
        SSmallPlate[Parameter::Interpolation]=0.0;
        SSmallPlate[Parameter::LateDiffuseCount]=0.4186999797821045;
        SSmallPlate[Parameter::LateDiffuseDelay]=0.2506999969482422;
        SSmallPlate[Parameter::LateDiffuseEnabled]=0.0;
        SSmallPlate[Parameter::LateDiffuseFeedback]=0.7119999527931213;
        SSmallPlate[Parameter::LateDiffuseModAmount]=0.2480999976396561;
        SSmallPlate[Parameter::LateDiffuseModRate]=0.1666999906301498;
        SSmallPlate[Parameter::LateLineCount]=0.8319999575614929;
        SSmallPlate[Parameter::LateLineDecay]=0.4959999918937683;
        SSmallPlate[Parameter::LateLineModAmount]=0.2719999849796295;
        SSmallPlate[Parameter::LateLineModRate]=0.2292999923229218;
        SSmallPlate[Parameter::LateLineSize]=0.543999969959259;
        SSmallPlate[Parameter::LateMode]=1.0;
        SSmallPlate[Parameter::LateOut]=0.5281000137329102;
        SSmallPlate[Parameter::HighCut]=0.687999963760376;
        SSmallPlate[Parameter::HighCutEnabled]=1.0;
        SSmallPlate[Parameter::SeedDelay]=0.2180999964475632;
        SSmallPlate[Parameter::SeedDiffusion]=0.1850000023841858;
        SSmallPlate[Parameter::SeedPostDiffusion]=0.3652999997138977;
        SSmallPlate[Parameter::SeedTap]=0.3339999914169312;
        SSmallPlate[Parameter::TapDecay]=1.0;
        SSmallPlate[Parameter::TapLength]=0.9866999983787537;
        SSmallPlate[Parameter::TapPredelay]=0.0;
        SSmallPlate[Parameter::TapCount]=0.1959999948740005;
        SSmallPlate[Parameter::TapEnabled]=0.0;

        SSnappyAttack[Parameter::DryOut]=1.0;
        SSnappyAttack[Parameter::EarlyDiffuseCount]=0.2119999974966049;
        SSnappyAttack[Parameter::EarlyDiffuseDelay]=0.0;
        SSnappyAttack[Parameter::EarlyDiffuseEnabled]=0.0;
        SSnappyAttack[Parameter::EarlyDiffuseFeedback]=0.7106999754905701;
        SSnappyAttack[Parameter::EarlyDiffuseModAmount]=0.2198999971151352;
        SSnappyAttack[Parameter::EarlyDiffuseModRate]=0.2466999888420105;
        SSnappyAttack[Parameter::EarlyOut]=0.371999979019165;
        SSnappyAttack[Parameter::EqCrossSeed]=0.0;
        SSnappyAttack[Parameter::EqCutoff]=0.9759999513626099;
        SSnappyAttack[Parameter::EqHighFreq]=0.5133999586105347;
        SSnappyAttack[Parameter::EqHighGain]=0.7680000066757202;
        SSnappyAttack[Parameter::EqHighShelfEnabled]=1.0;
        SSnappyAttack[Parameter::EqLowFreq]=0.3879999816417694;
        SSnappyAttack[Parameter::EqLowGain]=0.5559999942779541;
        SSnappyAttack[Parameter::EqLowShelfEnabled]=0.0;
        SSnappyAttack[Parameter::EqLowpassEnabled]=0.0;
        SSnappyAttack[Parameter::LowCut]=0.0;
        SSnappyAttack[Parameter::LowCutEnabled]=0.0;
        SSnappyAttack[Parameter::InputMix]=0.3026999831199646;
        SSnappyAttack[Parameter::Interpolation]=1.0;
        SSnappyAttack[Parameter::LateDiffuseCount]=0.03199999779462814;
        SSnappyAttack[Parameter::LateDiffuseDelay]=0.3479999899864197;
        SSnappyAttack[Parameter::LateDiffuseEnabled]=1.0;
        SSnappyAttack[Parameter::LateDiffuseFeedback]=0.7306999564170837;
        SSnappyAttack[Parameter::LateDiffuseModAmount]=0.2547999918460846;
        SSnappyAttack[Parameter::LateDiffuseModRate]=0.1666999906301498;
        SSnappyAttack[Parameter::LateLineCount]=0.3279999792575836;
        SSnappyAttack[Parameter::LateLineDecay]=0.2425999939441681;
        SSnappyAttack[Parameter::LateLineModAmount]=0.1159999966621399;
        SSnappyAttack[Parameter::LateLineModRate]=0.2292999923229218;
        SSnappyAttack[Parameter::LateLineSize]=0.3014000058174133;
        SSnappyAttack[Parameter::LateMode]=1.0;
        SSnappyAttack[Parameter::LateOut]=0.6319999694824219;
        SSnappyAttack[Parameter::HighCut]=1.0;
        SSnappyAttack[Parameter::HighCutEnabled]=0.0;
        SSnappyAttack[Parameter::SeedDelay]=0.09809999912977219;
        SSnappyAttack[Parameter::SeedDiffusion]=0.2549999952316284;
        SSnappyAttack[Parameter::SeedPostDiffusion]=0.189300000667572;
        SSnappyAttack[Parameter::SeedTap]=0.5879999995231628;
        SSnappyAttack[Parameter::TapDecay]=1.0;
        SSnappyAttack[Parameter::TapLength]=0.3226999938488007;
        SSnappyAttack[Parameter::TapPredelay]=0.0;
        SSnappyAttack[Parameter::TapCount]=0.5239999890327454;
        SSnappyAttack[Parameter::TapEnabled]=1.0;

        needParameterLoad = Parameter::COUNT;
    }

    void process(float32_t* inblockL, float32_t* inblockR, uint16_t len)
    {
        if (targetVol == 0.0f && vol > 0.0f)
        {
            engine.Process(inblockL, inblockR, inblockL, inblockR, len);
            for (uint16_t i = 0; i < len; ++i)
            {
                vol = std::max(0.0f, vol - ramp_dt);
                inblockL[i] *= vol;
                inblockR[i] *= vol;
            }

            return;
        }

        if (needBufferClear)
        {
            engine.StartSlowClear();
            needBufferClear = false;
            waitBufferClear = true;
        }

        if (waitBufferClear)
        {
            if (engine.SlowClearDone(SLOW_CLEAR_SIZE))
                waitBufferClear = false;

            memset(inblockL, 0, len * sizeof *inblockL);
            memset(inblockR, 0, len * sizeof *inblockR);

            return;
        }

        if (needParameterLoad)
        {
            unsigned needParam = needParameterLoad;
            unsigned paramID = Parameter::COUNT - needParam;
            engine.SetParameter(paramID, Presets[preset][paramID]);
            needParameterLoad = needParam - 1;
            
            memset(inblockL, 0, len * sizeof *inblockL);
            memset(inblockR, 0, len * sizeof *inblockR);

            if (enabled && !needParameterLoad)
                targetVol = 1.0f;

            return;
        }

        if (targetVol == 1.0f && vol < 1.0f)
        {
            engine.Process(inblockL, inblockR, inblockL, inblockR, len);
            for (uint16_t i = 0; i < len; ++i)
            {
                vol = std::min(vol + ramp_dt, 1.0f);
                inblockL[i] *= vol;
                inblockR[i] *= vol;
            }

            return;
        } 

        if (!enabled) return;

        engine.Process(inblockL, inblockR, inblockL, inblockR, len);
    }

    void setEnable(bool enable)
    {
        if (enabled && !enable)
        {
            targetVol = 0.0f;
            needBufferClear = true;
        }
        if (!enabled && enable)
        {
            targetVol = 1.0f;
        }
        enabled = enable;
    }

    void setPreset(unsigned p)
    {
        if (!enabled) return;

        preset = constrain(p, 0u, sizeof Presets / sizeof *Presets - 1);

        targetVol = 0.0f;
        needBufferClear = true;
        needParameterLoad = Parameter::COUNT;
    }

    void setRampedDown()
    {
        vol = 0.0f;
    }

private:
    float samplerate;
    float ramp_dt;
    Cloudseed::ReverbController engine;
    std::atomic<bool> enabled;
    std::atomic<float> targetVol;
    std::atomic<bool> needBufferClear;
    bool waitBufferClear;
    std::atomic<unsigned> needParameterLoad;
    std::atomic<unsigned> preset;

    std::atomic<float> vol;
};
