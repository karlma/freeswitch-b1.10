#define RX_PULSESHAPER_2400_GAIN        1.000000f
#define RX_PULSESHAPER_2400_COEFF_SETS  12
static const float rx_pulseshaper_2400_re[RX_PULSESHAPER_2400_COEFF_SETS][37] =
{
    {
           0.0015563115f,     /* Filter 0 */
          -0.0032627462f,
          -0.0018949322f,
           0.0078656419f,
          -0.0027306201f,
          -0.0069940380f,
           0.0056094252f,
           0.0010599786f,
           0.0020114800f,
          -0.0028990996f,
          -0.0149657275f,
           0.0234683618f,
           0.0124851225f,
          -0.0520524432f,
           0.0195473360f,
           0.0593000660f,
          -0.0659432016f,
          -0.0269855260f,
           0.0903420234f,
          -0.0279172205f,
          -0.0706490244f,
           0.0659432016f,
           0.0226506097f,
          -0.0632565081f,
           0.0160850895f,
           0.0326864751f,
          -0.0234683618f,
          -0.0057163992f,
           0.0093816835f,
          -0.0006215815f,
           0.0027750599f,
          -0.0056094252f,
          -0.0026714848f,
           0.0088364722f,
          -0.0024306170f,
          -0.0049609969f,
           0.0032627462f
    },
    {
           0.0016927688f,     /* Filter 1 */
          -0.0034082187f,
          -0.0019455259f,
           0.0079812854f,
          -0.0027410584f,
          -0.0069292139f,
           0.0054309843f,
           0.0009430216f,
           0.0025533503f,
          -0.0031144226f,
          -0.0156361473f,
           0.0242156763f,
           0.0127849906f,
          -0.0530130803f,
           0.0198226534f,
           0.0599172158f,
          -0.0664148260f,
          -0.0270975664f,
           0.0904586247f,
          -0.0278746829f,
          -0.0703398778f,
           0.0654580487f,
           0.0224109291f,
          -0.0623574663f,
           0.0157872820f,
           0.0319028797f,
          -0.0227260712f,
          -0.0054633388f,
           0.0086971142f,
          -0.0004583041f,
           0.0030702935f,
          -0.0057777402f,
          -0.0026929678f,
           0.0087945988f,
          -0.0023932450f,
          -0.0048265064f,
           0.0031172501f
    },
    {
           0.0018306919f,     /* Filter 2 */
          -0.0035535150f,
          -0.0019952730f,
           0.0080913986f,
          -0.0027489254f,
          -0.0068556597f,
           0.0052423164f,
           0.0008218653f,
           0.0031086542f,
          -0.0033334754f,
          -0.0163142578f,
           0.0249677455f,
           0.0130852984f,
          -0.0539702141f,
           0.0200953432f,
           0.0605237437f,
          -0.0668726942f,
          -0.0272035008f,
           0.0905540931f,
          -0.0278256577f,
          -0.0700150468f,
           0.0649596031f,
           0.0221674156f,
          -0.0614507444f,
           0.0154886502f,
           0.0311210713f,
          -0.0219890738f,
          -0.0052133718f,
           0.0080249289f,
          -0.0002992118f,
           0.0033546046f,
          -0.0059360333f,
          -0.0027112317f,
           0.0087448582f,
          -0.0023543099f,
          -0.0046901610f,
           0.0029718837f
    },
    {
           0.0019699621f,     /* Filter 3 */
          -0.0036984773f,
          -0.0020441043f,
           0.0081957427f,
          -0.0027541532f,
          -0.0067732266f,
           0.0050433272f,
           0.0006965003f,
           0.0036773273f,
          -0.0035562061f,
          -0.0169998438f,
           0.0257242871f,
           0.0133859229f,
          -0.0549234238f,
           0.0203652799f,
           0.0611193477f,
          -0.0673165844f,
          -0.0273032826f,
           0.0906283998f,
          -0.0277701685f,
          -0.0696746964f,
           0.0644481108f,
           0.0219201814f,
          -0.0605367479f,
           0.0151893226f,
           0.0303413603f,
          -0.0212576375f,
          -0.0049665736f,
           0.0073652682f,
          -0.0001443174f,
           0.0036280332f,
          -0.0060844136f,
          -0.0027263376f,
           0.0086874779f,
          -0.0023138848f,
          -0.0045521393f,
           0.0028267925f
    },
    {
           0.0021104569f,     /* Filter 4 */
          -0.0038429461f,
          -0.0020919492f,
           0.0082940836f,
          -0.0027566735f,
          -0.0066817710f,
           0.0048339281f,
           0.0005669188f,
           0.0042592917f,
          -0.0037825593f,
          -0.0176926866f,
           0.0264850193f,
           0.0136867395f,
          -0.0558722880f,
           0.0206323398f,
           0.0617037492f,
          -0.0677462761f,
          -0.0273968587f,
           0.0906815023f,
          -0.0277082472f,
          -0.0693189956f,
           0.0639238157f,
           0.0216693433f,
          -0.0596159016f,
           0.0148894283f,
           0.0295640577f,
          -0.0205320145f,
          -0.0047230166f,
           0.0067182711f,
           0.0000063691f,
           0.0038906329f,
          -0.0062229939f,
          -0.0027383462f,
           0.0086226877f,
          -0.0022720426f,
          -0.0044126151f,
           0.0026821236f
    },
    {
           0.0022520511f,     /* Filter 5 */
          -0.0039867608f,
          -0.0021387368f,
           0.0083861837f,
          -0.0027564187f,
          -0.0065811535f,
           0.0046140353f,
           0.0004331162f,
           0.0048544647f,
          -0.0040124783f,
          -0.0183925599f,
           0.0272496551f,
           0.0139876244f,
          -0.0568163868f,
           0.0208963983f,
           0.0622766679f,
          -0.0681615641f,
          -0.0274841848f,
           0.0907133744f,
          -0.0276399183f,
          -0.0689481154f,
           0.0633869663f,
           0.0214150209f,
          -0.0586886174f,
           0.0145890956f,
           0.0287894729f,
          -0.0198124637f,
          -0.0044827709f,
           0.0060840594f,
           0.0001528399f,
           0.0041424574f,
          -0.0063518949f,
          -0.0027473198f,
           0.0085507200f,
          -0.0022288557f,
          -0.0042717636f,
           0.0025380184f
    },
    {
           0.0023946154f,     /* Filter 6 */
          -0.0041297560f,
          -0.0021843964f,
           0.0084718067f,
          -0.0027533229f,
          -0.0064712394f,
           0.0043835688f,
           0.0002950896f,
           0.0054627541f,
          -0.0042459030f,
          -0.0190992307f,
           0.0280179056f,
           0.0142884518f,
          -0.0577553021f,
           0.0211573322f,
           0.0628378285f,
          -0.0685622430f,
          -0.0275652180f,
           0.0907240049f,
          -0.0275652180f,
          -0.0685622430f,
           0.0628378285f,
           0.0211573322f,
          -0.0577553021f,
           0.0142884518f,
           0.0280179056f,
          -0.0190992307f,
          -0.0042459030f,
           0.0054627541f,
           0.0002950896f,
           0.0043835688f,
          -0.0064712394f,
          -0.0027533229f,
           0.0084718067f,
          -0.0021843964f,
          -0.0041297560f,
           0.0023946154f
    },
    {
           0.0025380184f,     /* Filter 7 */
          -0.0042717636f,
          -0.0022288557f,
           0.0085507200f,
          -0.0027473198f,
          -0.0063518949f,
           0.0041424574f,
           0.0001528399f,
           0.0060840594f,
          -0.0044827709f,
          -0.0198124637f,
           0.0287894729f,
           0.0145890956f,
          -0.0586886174f,
           0.0214150209f,
           0.0633869663f,
          -0.0689481154f,
          -0.0276399183f,
           0.0907133744f,
          -0.0274841848f,
          -0.0681615641f,
           0.0622766679f,
           0.0208963983f,
          -0.0568163868f,
           0.0139876244f,
           0.0272496551f,
          -0.0183925599f,
          -0.0040124783f,
           0.0048544647f,
           0.0004331162f,
           0.0046140353f,
          -0.0065811535f,
          -0.0027564187f,
           0.0083861837f,
          -0.0021387368f,
          -0.0039867608f,
           0.0022520511f
    },
    {
           0.0026821236f,     /* Filter 8 */
          -0.0044126151f,
          -0.0022720426f,
           0.0086226877f,
          -0.0027383462f,
          -0.0062229939f,
           0.0038906329f,
           0.0000063691f,
           0.0067182711f,
          -0.0047230166f,
          -0.0205320145f,
           0.0295640577f,
           0.0148894283f,
          -0.0596159016f,
           0.0216693433f,
           0.0639238157f,
          -0.0693189956f,
          -0.0277082472f,
           0.0906815023f,
          -0.0273968587f,
          -0.0677462761f,
           0.0617037492f,
           0.0206323398f,
          -0.0558722880f,
           0.0136867395f,
           0.0264850193f,
          -0.0176926866f,
          -0.0037825593f,
           0.0042592917f,
           0.0005669188f,
           0.0048339281f,
          -0.0066817710f,
          -0.0027566735f,
           0.0082940836f,
          -0.0020919492f,
          -0.0038429461f,
           0.0021104569f
    },
    {
           0.0028267925f,     /* Filter 9 */
          -0.0045521393f,
          -0.0023138848f,
           0.0086874779f,
          -0.0027263376f,
          -0.0060844136f,
           0.0036280332f,
          -0.0001443174f,
           0.0073652682f,
          -0.0049665736f,
          -0.0212576375f,
           0.0303413603f,
           0.0151893226f,
          -0.0605367479f,
           0.0219201814f,
           0.0644481108f,
          -0.0696746964f,
          -0.0277701685f,
           0.0906283998f,
          -0.0273032826f,
          -0.0673165844f,
           0.0611193477f,
           0.0203652799f,
          -0.0549234238f,
           0.0133859229f,
           0.0257242871f,
          -0.0169998438f,
          -0.0035562061f,
           0.0036773273f,
           0.0006965003f,
           0.0050433272f,
          -0.0067732266f,
          -0.0027541532f,
           0.0081957427f,
          -0.0020441043f,
          -0.0036984773f,
           0.0019699621f
    },
    {
           0.0029718837f,     /* Filter 10 */
          -0.0046901610f,
          -0.0023543099f,
           0.0087448582f,
          -0.0027112317f,
          -0.0059360333f,
           0.0033546046f,
          -0.0002992118f,
           0.0080249289f,
          -0.0052133718f,
          -0.0219890738f,
           0.0311210713f,
           0.0154886502f,
          -0.0614507444f,
           0.0221674156f,
           0.0649596031f,
          -0.0700150468f,
          -0.0278256577f,
           0.0905540931f,
          -0.0272035008f,
          -0.0668726942f,
           0.0605237437f,
           0.0200953432f,
          -0.0539702141f,
           0.0130852984f,
           0.0249677455f,
          -0.0163142578f,
          -0.0033334754f,
           0.0031086542f,
           0.0008218653f,
           0.0052423164f,
          -0.0068556597f,
          -0.0027489254f,
           0.0080913986f,
          -0.0019952730f,
          -0.0035535150f,
           0.0018306919f
    },
    {
           0.0031172501f,     /* Filter 11 */
          -0.0048265064f,
          -0.0023932450f,
           0.0087945988f,
          -0.0026929678f,
          -0.0057777402f,
           0.0030702935f,
          -0.0004583041f,
           0.0086971142f,
          -0.0054633388f,
          -0.0227260712f,
           0.0319028797f,
           0.0157872820f,
          -0.0623574663f,
           0.0224109291f,
           0.0654580487f,
          -0.0703398778f,
          -0.0278746829f,
           0.0904586247f,
          -0.0270975664f,
          -0.0664148260f,
           0.0599172158f,
           0.0198226534f,
          -0.0530130803f,
           0.0127849906f,
           0.0242156763f,
          -0.0156361472f,
          -0.0031144226f,
           0.0025533503f,
           0.0009430216f,
           0.0054309843f,
          -0.0069292139f,
          -0.0027410584f,
           0.0079812854f,
          -0.0019455259f,
          -0.0034082187f,
           0.0016927688f
    }
};
static const float rx_pulseshaper_2400_im[RX_PULSESHAPER_2400_COEFF_SETS][37] =
{
    {
           0.0011307265f,     /* Filter 0 */
           0.0023705238f,
          -0.0058320016f,
           0.0000000000f,
           0.0084039844f,
          -0.0050814660f,
          -0.0040754860f,
           0.0032622786f,
           0.0000000000f,
           0.0089225112f,
          -0.0108732375f,
          -0.0170507629f,
           0.0384252560f,
          -0.0000000000f,
          -0.0601605142f,
           0.0430840199f,
           0.0479105404f,
          -0.0830529093f,
           0.0000000000f,
           0.0859203700f,
          -0.0513295208f,
          -0.0479105404f,
           0.0697114085f,
           0.0000000000f,
          -0.0495048153f,
           0.0237481142f,
           0.0170507629f,
          -0.0175932678f,
          -0.0000000000f,
           0.0019130312f,
           0.0020161991f,
           0.0040754860f,
          -0.0082219847f,
          -0.0000000000f,
           0.0074806699f,
          -0.0036043752f,
          -0.0023705238f
    },
    {
           0.0012298685f,     /* Filter 1 */
           0.0024762158f,
          -0.0059877130f,
           0.0000000000f,
           0.0084361101f,
          -0.0050343686f,
          -0.0039458410f,
           0.0029023221f,
           0.0000000000f,
           0.0095852072f,
          -0.0113603260f,
          -0.0175937187f,
           0.0393481552f,
          -0.0000000000f,
          -0.0610078541f,
           0.0435324055f,
           0.0482531955f,
          -0.0833977341f,
           0.0000000000f,
           0.0857894527f,
          -0.0511049127f,
          -0.0475580562f,
           0.0689737474f,
           0.0000000000f,
          -0.0485882579f,
           0.0231787989f,
           0.0165114572f,
          -0.0168144279f,
          -0.0000000000f,
           0.0014105150f,
           0.0022306988f,
           0.0041977740f,
          -0.0082881026f,
          -0.0000000000f,
           0.0073656508f,
          -0.0035066621f,
          -0.0022648148f
    },
    {
           0.0013300756f,     /* Filter 2 */
           0.0025817797f,
          -0.0061408189f,
           0.0000000000f,
           0.0084603224f,
          -0.0049809284f,
          -0.0038087658f,
           0.0025294414f,
           0.0000000000f,
           0.0102593823f,
          -0.0118530021f,
          -0.0181401289f,
           0.0402724075f,
          -0.0000000000f,
          -0.0618471069f,
           0.0439730737f,
           0.0485858563f,
          -0.0837237667f,
           0.0000000000f,
           0.0856385688f,
          -0.0508689091f,
          -0.0471959142f,
           0.0682242900f,
           0.0000000000f,
          -0.0476691639f,
           0.0226107818f,
           0.0159759973f,
          -0.0160451085f,
          -0.0000000000f,
           0.0009208793f,
           0.0024372629f,
           0.0043127806f,
          -0.0083443130f,
          -0.0000000000f,
           0.0072458207f,
          -0.0034076014f,
          -0.0021591999f
    },
    {
           0.0014312612f,     /* Filter 3 */
           0.0026871011f,
          -0.0062911061f,
           0.0000000000f,
           0.0084764120f,
          -0.0049210372f,
          -0.0036641917f,
           0.0021436074f,
           0.0000000000f,
           0.0109448771f,
          -0.0123511095f,
          -0.0186897886f,
           0.0411976345f,
          -0.0000000000f,
          -0.0626778865f,
           0.0444058054f,
           0.0489083614f,
          -0.0840308634f,
           0.0000000000f,
           0.0854677905f,
          -0.0506216301f,
          -0.0468242934f,
           0.0674633814f,
           0.0000000000f,
          -0.0467479280f,
           0.0220442886f,
           0.0154445777f,
          -0.0152855418f,
          -0.0000000000f,
           0.0004441633f,
           0.0026359204f,
           0.0044205852f,
          -0.0083908043f,
          -0.0000000000f,
           0.0071214052f,
          -0.0033073228f,
          -0.0020537850f
    },
    {
           0.0015333367f,     /* Filter 4 */
           0.0027920638f,
          -0.0064383577f,
           0.0000000000f,
           0.0084841685f,
          -0.0048545908f,
          -0.0035120543f,
           0.0017447968f,
           0.0000000000f,
           0.0116415204f,
          -0.0128544893f,
          -0.0192424929f,
           0.0421234528f,
          -0.0000000000f,
          -0.0634998126f,
           0.0448303979f,
           0.0492205507f,
          -0.0843188611f,
           0.0000000000f,
           0.0852772163f,
          -0.0503631983f,
          -0.0464433707f,
           0.0666913811f,
           0.0000000000f,
          -0.0458249485f,
           0.0214795452f,
           0.0149173817f,
          -0.0145359503f,
          -0.0000000000f,
          -0.0000196020f,
           0.0028267103f,
           0.0045212697f,
          -0.0084277630f,
          -0.0000000000f,
           0.0069926280f,
          -0.0032059525f,
          -0.0019486769f
    },
    {
           0.0016362109f,     /* Filter 5 */
           0.0028965513f,
          -0.0065823550f,
           0.0000000000f,
           0.0084833845f,
          -0.0047814879f,
          -0.0033522929f,
           0.0013329947f,
           0.0000000000f,
           0.0123491384f,
          -0.0133629770f,
          -0.0197980333f,
           0.0430494814f,
          -0.0000000000f,
          -0.0643125010f,
           0.0452466478f,
           0.0495222751f,
          -0.0845876231f,
           0.0000000000f,
           0.0850669216f,
          -0.0500937381f,
          -0.0460533268f,
           0.0659086572f,
           0.0000000000f,
          -0.0449006193f,
           0.0209167764f,
           0.0143945974f,
          -0.0137965502f,
          -0.0000000000f,
          -0.0004703930f,
           0.0030096715f,
           0.0046149218f,
          -0.0084553808f,
          -0.0000000000f,
           0.0068597125f,
          -0.0031036179f,
          -0.0018439783f
    },
    {
           0.0017397900f,     /* Filter 6 */
           0.0030004433f,
          -0.0067228808f,
           0.0000000000f,
           0.0084738564f,
          -0.0047016307f,
          -0.0031848491f,
           0.0009081924f,
           0.0000000000f,
           0.0130675456f,
          -0.0138764034f,
          -0.0203562000f,
           0.0439753330f,
          -0.0000000000f,
          -0.0651155730f,
           0.0456543548f,
           0.0498133853f,
          -0.0848370177f,
           0.0000000000f,
           0.0848370177f,
          -0.0498133853f,
          -0.0456543548f,
           0.0651155730f,
           0.0000000000f,
          -0.0439753330f,
           0.0203562000f,
           0.0138764034f,
          -0.0130675456f,
          -0.0000000000f,
          -0.0009081924f,
           0.0031848491f,
           0.0047016307f,
          -0.0084738564f,
          -0.0000000000f,
           0.0067228808f,
          -0.0030004433f,
          -0.0017397900f
    },
    {
           0.0018439783f,     /* Filter 7 */
           0.0031036179f,
          -0.0068597125f,
           0.0000000000f,
           0.0084553808f,
          -0.0046149218f,
          -0.0030096715f,
           0.0004703930f,
           0.0000000000f,
           0.0137965502f,
          -0.0143945974f,
          -0.0209167764f,
           0.0449006193f,
          -0.0000000000f,
          -0.0659086573f,
           0.0460533268f,
           0.0500937381f,
          -0.0850669216f,
           0.0000000000f,
           0.0845876231f,
          -0.0495222751f,
          -0.0452466478f,
           0.0643125010f,
           0.0000000000f,
          -0.0430494814f,
           0.0197980333f,
           0.0133629770f,
          -0.0123491384f,
          -0.0000000000f,
          -0.0013329947f,
           0.0033522929f,
           0.0047814879f,
          -0.0084833845f,
          -0.0000000000f,
           0.0065823550f,
          -0.0028965513f,
          -0.0016362109f
    },
    {
           0.0019486769f,     /* Filter 8 */
           0.0032059525f,
          -0.0069926280f,
           0.0000000000f,
           0.0084277630f,
          -0.0045212697f,
          -0.0028267103f,
           0.0000196020f,
           0.0000000000f,
           0.0145359503f,
          -0.0149173817f,
          -0.0214795452f,
           0.0458249485f,
          -0.0000000000f,
          -0.0666913811f,
           0.0464433707f,
           0.0503631983f,
          -0.0852772163f,
           0.0000000000f,
           0.0843188611f,
          -0.0492205507f,
          -0.0448303979f,
           0.0634998126f,
           0.0000000000f,
          -0.0421234528f,
           0.0192424929f,
           0.0128544893f,
          -0.0116415204f,
          -0.0000000000f,
          -0.0017447968f,
           0.0035120543f,
           0.0048545908f,
          -0.0084841685f,
          -0.0000000000f,
           0.0064383577f,
          -0.0027920638f,
          -0.0015333367f
    },
    {
           0.0020537850f,     /* Filter 9 */
           0.0033073228f,
          -0.0071214052f,
           0.0000000000f,
           0.0083908043f,
          -0.0044205852f,
          -0.0026359204f,
          -0.0004441633f,
           0.0000000000f,
           0.0152855418f,
          -0.0154445777f,
          -0.0220442886f,
           0.0467479280f,
          -0.0000000000f,
          -0.0674633814f,
           0.0468242934f,
           0.0506216301f,
          -0.0854677905f,
           0.0000000000f,
           0.0840308634f,
          -0.0489083614f,
          -0.0444058054f,
           0.0626778865f,
           0.0000000000f,
          -0.0411976344f,
           0.0186897886f,
           0.0123511095f,
          -0.0109448771f,
          -0.0000000000f,
          -0.0021436074f,
           0.0036641917f,
           0.0049210372f,
          -0.0084764120f,
          -0.0000000000f,
           0.0062911061f,
          -0.0026871011f,
          -0.0014312612f
    },
    {
           0.0021591999f,     /* Filter 10 */
           0.0034076014f,
          -0.0072458207f,
           0.0000000000f,
           0.0083443130f,
          -0.0043127806f,
          -0.0024372629f,
          -0.0009208793f,
           0.0000000000f,
           0.0160451085f,
          -0.0159759973f,
          -0.0226107818f,
           0.0476691639f,
          -0.0000000000f,
          -0.0682242900f,
           0.0471959142f,
           0.0508689091f,
          -0.0856385688f,
           0.0000000000f,
           0.0837237667f,
          -0.0485858563f,
          -0.0439730737f,
           0.0618471068f,
           0.0000000000f,
          -0.0402724075f,
           0.0181401289f,
           0.0118530021f,
          -0.0102593823f,
          -0.0000000000f,
          -0.0025294414f,
           0.0038087658f,
           0.0049809284f,
          -0.0084603224f,
          -0.0000000000f,
           0.0061408189f,
          -0.0025817797f,
          -0.0013300756f
    },
    {
           0.0022648148f,     /* Filter 11 */
           0.0035066621f,
          -0.0073656508f,
           0.0000000000f,
           0.0082881026f,
          -0.0041977740f,
          -0.0022306988f,
          -0.0014105150f,
           0.0000000000f,
           0.0168144279f,
          -0.0165114572f,
          -0.0231787989f,
           0.0485882579f,
          -0.0000000000f,
          -0.0689737474f,
           0.0475580562f,
           0.0511049127f,
          -0.0857894527f,
           0.0000000000f,
           0.0833977341f,
          -0.0482531955f,
          -0.0435324055f,
           0.0610078541f,
           0.0000000000f,
          -0.0393481552f,
           0.0175937187f,
           0.0113603260f,
          -0.0095852072f,
          -0.0000000000f,
          -0.0029023221f,
           0.0039458410f,
           0.0050343686f,
          -0.0084361101f,
          -0.0000000000f,
           0.0059877130f,
          -0.0024762158f,
          -0.0012298685f
    }
};
