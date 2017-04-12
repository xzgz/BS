/* Copyright 2003 Roger P. Woods, M.D. */
/* Modified 2/8/03 */

/*
 * This routine computes the order of Pade approximation (<=16) needed to compute
 * log(1+x) to a specified degree of accuracy
 *
 * Although applied here to a scalar quantity, setting x equal to the 1-norm of a 
 * corresponding matrix X will assure that the 1-norm of the log of (I-X) will also
 * have the specified degree of accuracy. This statement is equivalent to equation
 * (6.1) on page 1120 of Cheng, et al, Approximating the logarithm of a matrix to specified accuracy.
 * SIAM J. Matrix Anal. Appl. 2001;22(4):1112-1125.
 *
 * The computation is based on equation (7.3) of Cheng, et al, which is on page 1121.
 *
 * The nodes and weights of the Gauss-Legendre quadrature rule on [0,1] were precomputed using long
 * double precision
 *
 * On input:
 *
 * 	X is the value for which log(1+X) is to be approximated
 * 	EPSILON is the accuracy to which log(1+X) is desired
 *
 * Returns the order of Pade approximation needed or zero if the desired accuracy cannot be
 * attained
 */

#include "AIR.h"

unsigned int AIR_padeorder(const double x, double epsilon){

	// The precalculated Gauss-Legendre nodes for m=1 to 16, computed with long double precision

	const double nodes[]={
		5.0000000000000000000000000000000000e-01,
		2.1132486540518711774542560974902000e-01,
		7.8867513459481288225457439025097690e-01,
		1.1270166537925831148207346002176160e-01,
		5.0000000000000000000000000000000000e-01,
		8.8729833462074168851792653997823840e-01,
		6.9431844202973712388026755553595180e-02,
		3.3000947820757186759866712044837840e-01,
		6.6999052179242813240133287955162470e-01,
		9.3056815579702628761197324444640170e-01,
		4.6910077030668003601186560850303490e-02,
		2.3076534494715845448184278964989590e-01,
		5.0000000000000000000000000000000000e-01,
		7.6923465505284154551815721035010100e-01,
		9.5308992296933199639881343914969650e-01,
		3.3765242898423986093849222753002080e-02,
		1.6939530676686774316930020249004850e-01,
		3.8069040695840154568474913915964310e-01,
		6.1930959304159845431525086084036000e-01,
		8.3060469323313225683069979750994840e-01,
		9.6623475710157601390615077724699790e-01,
		2.5446043828620737736905157976075020e-02,
		1.2923440720030278006806761335960850e-01,
		2.9707742431130141654669679396151970e-01,
		5.0000000000000000000000000000000000e-01,
		7.0292257568869858345330320603848030e-01,
		8.7076559279969721993193238664039150e-01,
		9.7455395617137926226309484202392190e-01,
		1.9855071751231884158219565715262600e-02,
		1.0166676129318663020422303176208400e-01,
		2.3723379504183550709113047540537740e-01,
		4.0828267875217509753026192881990820e-01,
		5.9171732124782490246973807118009180e-01,
		7.6276620495816449290886952459462570e-01,
		8.9833323870681336979577696823791600e-01,
		9.8014492824876811584178043428473740e-01,
		1.5919880246186955082211898548163550e-02,
		8.1984446336682102850285105965130890e-02,
		1.9331428364970480134564898032926190e-01,
		3.3787328829809553548073099267833190e-01,
		5.0000000000000000000000000000000000e-01,
		6.6212671170190446451926900732166500e-01,
		8.0668571635029519865435101967073810e-01,
		9.1801555366331789714971489403486910e-01,
		9.8408011975381304491778810145183950e-01,
		1.3046735741414139961017993957773030e-02,
		6.7468316655507744633951655788256160e-02,
		1.6029521585048779688283631744256260e-01,
		2.8330230293537640460036702841710760e-01,
		4.2556283050918439455758699943513930e-01,
		5.7443716949081560544241300056486070e-01,
		7.1669769706462359539963297158288930e-01,
		8.3970478414951220311716368255743740e-01,
		9.3253168334449225536604834421174380e-01,
		9.8695326425858586003898200604222700e-01,
		1.0885670926971503598030999438570100e-02,
		5.6468700115952350462421115348034570e-02,
		1.3492399721297533795329187398442170e-01,
		2.4045193539659409203713716527069400e-01,
		3.6522842202382751383423400729956860e-01,
		5.0000000000000000000000000000000000e-01,
		6.3477157797617248616576599270043140e-01,
		7.5954806460340590796286283472930600e-01,
		8.6507600278702466204670812601557830e-01,
		9.4353129988404764953757888465196540e-01,
		9.8911432907302849640196900056142680e-01,
		9.2196828766403746547254549253614290e-03,
		4.7941371814762571660767066940452000e-02,
		1.1504866290284765648155308339359270e-01,
		2.0634102285669127635164879052973290e-01,
		3.1608425050090990312365423167814110e-01,
		4.3738329574426554226377931526807490e-01,
		5.6261670425573445773622068473192510e-01,
		6.8391574949909009687634576832185580e-01,
		7.9365897714330872364835120947026400e-01,
		8.8495133709715234351844691660640730e-01,
		9.5205862818523742833923293305955110e-01,
		9.9078031712335962534527454507463860e-01,
		7.9084726407059252635852755964476790e-03,
		4.1200800388511017396726081749642380e-02,
		9.9210954633345043602896755208570380e-02,
		1.7882533027982988967800769650224270e-01,
		2.7575362448177657356104357393617960e-01,
		3.8477084202243260296723593945100380e-01,
		5.0000000000000000000000000000000000e-01,
		6.1522915797756739703276406054899620e-01,
		7.2424637551822342643895642606382340e-01,
		8.2117466972017011032199230349776040e-01,
		9.0078904536665495639710324479143270e-01,
		9.5879919961148898260327391825035760e-01,
		9.9209152735929407473641472440355230e-01,
		6.8580956515938305792013666479716210e-03,
		3.5782558168213241331804430311064040e-02,
		8.6399342465117503405102628674804530e-02,
		1.5635354759415726492599009849033270e-01,
		2.4237568182092295401735464072440510e-01,
		3.4044381553605511978216408791576230e-01,
		4.4597252564632816896687767489008340e-01,
		5.5402747435367183103312232510991970e-01,
		6.5955618446394488021783591208423770e-01,
		7.5762431817907704598264535927559800e-01,
		8.4364645240584273507400990150967040e-01,
		9.1360065753488249659489737132519550e-01,
		9.6421744183178675866819556968893600e-01,
		9.9314190434840616942079863335202840e-01,
		6.0037409897572857552171407066950410e-03,
		3.1363303799647047846120526144894280e-02,
		7.5896708294786391899675839612891670e-02,
		1.3779113431991497629190697269303150e-01,
		2.1451391369573057623138663137304480e-01,
		3.0292432646121831505139631450947650e-01,
		3.9940295300128273884968584830270200e-01,
		5.0000000000000000000000000000000000e-01,
		6.0059704699871726115031415169729800e-01,
		6.9707567353878168494860368549052350e-01,
		7.8548608630426942376861336862695830e-01,
		8.6220886568008502370809302730696850e-01,
		9.2410329170521360810032416038710830e-01,
		9.6863669620035295215387947385510570e-01,
		9.9399625901024271424478285929330800e-01,
		5.2995325041750337019229132748351560e-03,
		2.7712488463383711961005792232695320e-02,
		6.7184398806084128059766051143804700e-02,
		1.2229779582249848305244940257628010e-01,
		1.9106187779867812577666411797560360e-01,
		2.7099161117138630682879027850820970e-01,
		3.5919822461037054338476974926975320e-01,
		4.5249374508118127990734033228752130e-01,
		5.4750625491881872009265966771247870e-01,
		6.4080177538962945661523025073024680e-01,
		7.2900838882861369317120972149179030e-01,
		8.0893812220132187422333588202439950e-01,
		8.7770220417750151694755059742371690e-01,
		9.3281560119391587194023394885619840e-01,
		9.7228751153661628803899420776730470e-01,
		9.9470046749582496629807708672516180e-01
	};
	
	// The precalculated Gauss-Legendre weights for m=1 to 16, computed with long double precision

	const double weights[]={
		1.0000000000000000000000000000000000e+00,
		5.0000000000000000000000000000001230e-01,
		5.0000000000000000000000000000001230e-01,
		2.7777777777777777777777777777779390e-01,
		4.4444444444444444444444444444444310e-01,
		2.7777777777777777777777777777779390e-01,
		1.7392742256872692868653197461100380e-01,
		3.2607257743127307131346802538898850e-01,
		3.2607257743127307131346802538898850e-01,
		1.7392742256872692868653197461100380e-01,
		1.1846344252809454375713202035996630e-01,
		2.3931433524968323402064575741781380e-01,
		2.8444444444444444444444444444444300e-01,
		2.3931433524968323402064575741781380e-01,
		1.1846344252809454375713202035996630e-01,
		8.5662246189585172520148071086364560e-02,
		1.8038078652406930378491675691886590e-01,
		2.3395696728634552369493517199477880e-01,
		2.3395696728634552369493517199477880e-01,
		1.8038078652406930378491675691886590e-01,
		8.5662246189585172520148071086364560e-02,
		6.4742483084434846635305716339534610e-02,
		1.3985269574463833395073388571188040e-01,
		1.9091502525255947247518488774449360e-01,
		2.0897959183673469387755102040816280e-01,
		1.9091502525255947247518488774449360e-01,
		1.3985269574463833395073388571188040e-01,
		6.4742483084434846635305716339534610e-02,
		5.0614268145188129576265677154980430e-02,
		1.1119051722668723527217799721311960e-01,
		1.5685332293894364366898110099329100e-01,
		1.8134189168918099148257522463860390e-01,
		1.8134189168918099148257522463860390e-01,
		1.5685332293894364366898110099329100e-01,
		1.1119051722668723527217799721311960e-01,
		5.0614268145188129576265677154980430e-02,
		4.0637194180787205985946079055258670e-02,
		9.0324080347428702029236015621457550e-02,
		1.3030534820146773115937143470933080e-01,
		1.5617353852000142003431520329222160e-01,
		1.6511967750062988158226253464348660e-01,
		1.5617353852000142003431520329222160e-01,
		1.3030534820146773115937143470933080e-01,
		9.0324080347428702029236015621457550e-02,
		4.0637194180787205985946079055258670e-02,
		3.3335672154344068796784404946658560e-02,
		7.4725674575290296572888169828855870e-02,
		1.0954318125799102199776746711408280e-01,
		1.3463335965499817754561346078473600e-01,
		1.4776211235737643508694649732566370e-01,
		1.4776211235737643508694649732566370e-01,
		1.3463335965499817754561346078473600e-01,
		1.0954318125799102199776746711408280e-01,
		7.4725674575290296572888169828855870e-02,
		3.3335672154344068796784404946658560e-02,
		2.7834283558086833241376860221288700e-02,
		6.2790184732452312317347149611963190e-02,
		9.3145105463867125713048820715827430e-02,
		1.1659688229599523995926185242158740e-01,
		1.3140227225512333109034443494525210e-01,
		1.3646254338895031535724176416817160e-01,
		1.3140227225512333109034443494525210e-01,
		1.1659688229599523995926185242158740e-01,
		9.3145105463867125713048820715827430e-02,
		6.2790184732452312317347149611963190e-02,
		2.7834283558086833241376860221288700e-02,
		2.3587668193255913597307980742515730e-02,
		5.3469662997659215480127359096998450e-02,
		8.0039164271673113167326264771680450e-02,
		1.0158371336153296087453222790489100e-01,
		1.1674626826917740438042494946244330e-01,
		1.2457352290670139250028121802147410e-01,
		1.2457352290670139250028121802147410e-01,
		1.1674626826917740438042494946244330e-01,
		1.0158371336153296087453222790489100e-01,
		8.0039164271673113167326264771680450e-02,
		5.3469662997659215480127359096998450e-02,
		2.3587668193255913597307980742515730e-02,
		2.0242002382657939760010796100505640e-02,
		4.6060749918864223957210887976894990e-02,
		6.9436755109893619231800888434433080e-02,
		8.9072990380972869140023345998051160e-02,
		1.0390802376844425115626160965302540e-01,
		1.1314159013144861920604509301988250e-01,
		1.1627577661543695509729475763441760e-01,
		1.1314159013144861920604509301988250e-01,
		1.0390802376844425115626160965302540e-01,
		8.9072990380972869140023345998051160e-02,
		6.9436755109893619231800888434433080e-02,
		4.6060749918864223957210887976894990e-02,
		2.0242002382657939760010796100505640e-02,
		1.7559730165875931515916438069094010e-02,
		4.0079043579880104902816638531429040e-02,
		6.0759285343951592344707404536230100e-02,
		7.8601583579096767284800969311928440e-02,
		9.2769198738968906870858295062580450e-02,
		1.0259923186064780198296203283060810e-01,
		1.0763192673157889509793822165813000e-01,
		1.0763192673157889509793822165813000e-01,
		1.0259923186064780198296203283060810e-01,
		9.2769198738968906870858295062580450e-02,
		7.8601583579096767284800969311928440e-02,
		6.0759285343951592344707404536230100e-02,
		4.0079043579880104902816638531429040e-02,
		1.7559730165875931515916438069094010e-02,
		1.5376620998058634177314196788614650e-02,
		3.5183023744054062354633708225335260e-02,
		5.3579610233585967505934773342933350e-02,
		6.9785338963077157223902397255506200e-02,
		8.3134602908496966776600430240607020e-02,
		9.3080500007781105513400280933212280e-02,
		9.9215742663555788228059163221920460e-02,
		1.0128912096278063644031009998375970e-01,
		9.9215742663555788228059163221920460e-02,
		9.3080500007781105513400280933212280e-02,
		8.3134602908496966776600430240607020e-02,
		6.9785338963077157223902397255506200e-02,
		5.3579610233585967505934773342933350e-02,
		3.5183023744054062354633708225335260e-02,
		1.5376620998058634177314196788614650e-02,
		1.3576229705877047425890286228014620e-02,
		3.1126761969323946431421918497188420e-02,
		4.7579255841246392404962553801126220e-02,
		6.2314485627766936026238141096014070e-02,
		7.4797994408288366040750865273738020e-02,
		8.4578259697501269094656039515179620e-02,
		9.1301707522461794433381833984611900e-02,
		9.4725305227534248142698361604144080e-02,
		9.4725305227534248142698361604144080e-02,
		9.1301707522461794433381833984611900e-02,
		8.4578259697501269094656039515179620e-02,
		7.4797994408288366040750865273738020e-02,
		6.2314485627766936026238141096014070e-02,
		4.7579255841246392404962553801126220e-02,
		3.1126761969323946431421918497188420e-02,
		1.3576229705877047425890286228014620e-02
	};
	{
		unsigned int k;
		
		for(k=1;k<=16;k++){
			
			const unsigned int offset=k*(k-1)/2;
			const double *node_ptr=nodes+offset;
			const double *weight_ptr=weights+offset;
			
			// Initialize with the first Pade term
			double rr=weight_ptr[0]*x/(1.0+node_ptr[0]*x); // j=1 term thus omitted below
			
			unsigned int j;
			
			// Add the remaining Pade terms in reverse order to reduce roundoff errors
			for(j=k;j>1;j--){	// Sum in this order to reduce round-off error
							
				rr+=weight_ptr[j-1]*x/(1.0+node_ptr[j-1]*x);
			}
			if(fabs(rr-log(1.0+x))<=epsilon){
				return k;
			}
		}
		return 0;
	}
}
