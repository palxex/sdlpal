/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Perl Artistic License, available in COPYING.
*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include "timidity/common.h"

#include "tables.h"

const Sint32 freq_table[128]=
{
 8176, 8662, 9177, 9723, 
 10301, 10913, 11562, 12250, 
 12978, 13750, 14568, 15434,
 
 16352, 17324, 18354, 19445,
 20602, 21827, 23125, 24500, 
 25957, 27500, 29135, 30868, 

 32703, 34648, 36708, 38891,
 41203, 43654, 46249, 48999,
 51913, 55000, 58270, 61735,

 65406, 69296, 73416, 77782,
 82407, 87307, 92499, 97999,
 103826, 110000, 116541, 123471,

 130813, 138591, 146832, 155563,
 164814, 174614, 184997, 195998,
 207652, 220000, 233082, 246942,

 261626, 277183, 293665, 311127,
 329628, 349228, 369994, 391995,
 415305, 440000, 466164, 493883,

 523251, 554365, 587330, 622254,
 659255, 698456, 739989, 783991,
 830609, 880000, 932328, 987767,

 1046502, 1108731, 1174659, 1244508,
 1318510, 1396913, 1479978, 1567982,
 1661219, 1760000, 1864655, 1975533,

 2093005, 2217461, 2349318, 2489016,
 2637020, 2793826, 2959955, 3135963,
 3322438, 3520000, 3729310, 3951066,

 4186009, 4434922, 4698636, 4978032,
 5274041, 5587652, 5919911, 6271927,
 6644875, 7040000, 7458620, 7902133,

 8372018, 8869844, 9397273, 9956063, 
 10548082, 11175303, 11839822, 12543854
};

/* v=2.^((x/127-1) * 6) */
const double vol_table[128] = 
{
 0.015625, 0.016145143728351113, 0.016682602624583379, 0.017237953096759438,
 0.017811790741104401, 0.01840473098076444, 0.019017409725829021, 0.019650484055324921,
 0.020304632921913132, 0.020980557880044631, 0.021678983838355849, 0.02240065983711079,
 0.023146359851523596, 0.023916883621822989, 0.024713057510949051, 0.025535735390801884,
 0.026385799557992876, 0.027264161680080529, 0.028171763773305786, 0.029109579212875332,
 0.030078613776876421, 0.031079906724942836, 0.032114531912828696, 0.033183598944085631,
 0.034288254360078256, 0.035429682869614412, 0.036609108619508737, 0.037827796507442342,
 0.039087053538526394, 0.040388230227024875, 0.041732722044739302, 0.043121970917609151,
 0.044557466772132896, 0.046040749133268132, 0.047573408775524545, 0.049157089429020417,
 0.050793489542332405, 0.05248436410402918, 0.054231526524842463, 0.056036850582493913,
 0.057902272431264008, 0.059829792678457581, 0.061821478529993396, 0.063879466007418645,
 0.066005962238725971, 0.068203247825430205, 0.070473679288442961, 0.072819691595368496,
 0.075243800771931268, 0.077748606600335793, 0.080336795407452768, 0.083011142945821612,
 0.085774517370559328, 0.088629882315368294, 0.091580300070941839, 0.094628934869176312,
 0.097779056276712184, 0.10103404270144323, 0.1043973850157546, 0.1078726903003755,
 0.11146368571286204, 0.11517422248485852, 0.11900828005242428, 0.12296997032385605,
 0.12706354208958254, 0.13129338557886089, 0.13566403716816194, 0.14018018424629392,
 0.14484667024148207, 0.14966849981579558, 0.15465084423249356, 0.15979904690204472,
 0.16511862911277009, 0.17061529595225433, 0.17629494242587571, 0.18216365977901747,
 0.18822774202974024, 0.19449369271892172, 0.20096823188510385, 0.20765830327152621,
 0.21457108177307616, 0.22171398113114205, 0.2290946618846218, 0.23672103958561411,
 0.2446012932886038, 0.25274387432224471, 0.26115751535314891, 0.26985123975140174,
 0.27883437126784744, 0.28811654403352405, 0.29770771289197112, 0.30761816407549192,
 0.31785852623682015, 0.32843978184802081, 0.33937327897885317, 0.3506707434672246,
 0.36234429149478936, 0.37440644258117928, 0.38687013301080181, 0.39974872970660535,
 0.41305604456569134, 0.42680634927214656, 0.44101439060298442, 0.45569540624360722,
 0.47086514112975281, 0.48653986433345225, 0.50273638651110641, 0.51947207793239625,
 0.53676488710936021, 0.55463336004561792, 0.57309666012638816, 0.59217458867062556,
 0.61188760616732485, 0.63225685421876243, 0.65330417821421161, 0.67505215075844849,
 0.69752409588017272, 0.72074411404630734, 0.74473710800900605, 0.76952880951308478,
 0.79514580689252357, 0.82161557358563286, 0.84896649759946774, 0.87722791195508854,
 0.90643012614631979, 0.93660445864574493, 0.96778327049280244, 1
};

const double bend_fine[256] = {
 1, 1.0002256593050698, 1.0004513695322617, 1.0006771306930664, 
 1.0009029427989777, 1.0011288058614922, 1.0013547198921082, 1.0015806849023274,
 1.0018067009036538, 1.002032767907594, 1.0022588859256572, 1.0024850549693551,
 1.0027112750502025, 1.0029375461797159, 1.0031638683694153, 1.0033902416308227,
 1.0036166659754628, 1.0038431414148634, 1.0040696679605541, 1.0042962456240678,
 1.0045228744169397, 1.0047495543507072, 1.0049762854369111, 1.0052030676870944,
 1.0054299011128027, 1.0056567857255843, 1.00588372153699, 1.006110708558573,
 1.0063377468018897, 1.0065648362784985, 1.0067919769999607, 1.0070191689778405,
 1.0072464122237039, 1.0074737067491204, 1.0077010525656616, 1.0079284496849015,
 1.0081558981184175, 1.008383397877789, 1.008610948974598, 1.0088385514204294,
 1.0090662052268706, 1.0092939104055114, 1.0095216669679448, 1.0097494749257656,
 1.009977334290572, 1.0102052450739643, 1.0104332072875455, 1.0106612209429215,
 1.0108892860517005, 1.0111174026254934, 1.0113455706759138, 1.0115737902145781,
 1.0118020612531047, 1.0120303838031153, 1.0122587578762337, 1.012487183484087,
 1.0127156606383041, 1.0129441893505169, 1.0131727696323602, 1.0134014014954713,
 1.0136300849514894, 1.0138588200120575, 1.0140876066888203, 1.0143164449934257,
 1.0145453349375237, 1.0147742765327674, 1.0150032697908125, 1.0152323147233171,
 1.015461411341942, 1.0156905596583505, 1.0159197596842091, 1.0161490114311862,
 1.0163783149109531, 1.0166076701351838, 1.0168370771155553, 1.0170665358637463,
 1.0172960463914391, 1.0175256087103179, 1.0177552228320703, 1.0179848887683858,
 1.0182146065309567, 1.0184443761314785, 1.0186741975816487, 1.0189040708931674,
 1.0191339960777379, 1.0193639731470658, 1.0195940021128593, 1.0198240829868295,
 1.0200542157806898, 1.0202844005061564, 1.0205146371749483, 1.0207449257987866,
 1.0209752663893958, 1.0212056589585028, 1.0214361035178368, 1.0216666000791297,
 1.0218971486541166, 1.0221277492545349, 1.0223584018921241, 1.0225891065786274,
 1.0228198633257899, 1.0230506721453596, 1.023281533049087, 1.0235124460487257,
 1.0237434111560313, 1.0239744283827625, 1.0242054977406807, 1.0244366192415495,
 1.0246677928971357, 1.0248990187192082, 1.025130296719539, 1.0253616269099028,
 1.0255930093020766, 1.0258244439078401, 1.0260559307389761, 1.0262874698072693,
 1.0265190611245079, 1.0267507047024822, 1.0269824005529853, 1.027214148687813,
 1.0274459491187637, 1.0276778018576387, 1.0279097069162415, 1.0281416643063788,
 1.0283736740398595, 1.0286057361284953, 1.0288378505841009, 1.0290700174184932,
 1.0293022366434921, 1.0295345082709197, 1.0297668323126017, 1.0299992087803651,
 1.030231637686041, 1.0304641190414621, 1.0306966528584645, 1.0309292391488862,
 1.0311618779245688, 1.0313945691973556, 1.0316273129790936, 1.0318601092816313,
 1.0320929581168212, 1.0323258594965172, 1.0325588134325767, 1.0327918199368598,
 1.0330248790212284, 1.0332579906975481, 1.0334911549776868, 1.033724371873515,
 1.0339576413969056, 1.0341909635597348, 1.0344243383738811, 1.0346577658512259,
 1.034891246003653, 1.0351247788430489, 1.0353583643813031, 1.0355920026303078,
 1.0358256936019572, 1.0360594373081489, 1.0362932337607829, 1.0365270829717617,
 1.0367609849529913, 1.0369949397163791, 1.0372289472738365, 1.0374630076372766,
 1.0376971208186156, 1.0379312868297725, 1.0381655056826686, 1.0383997773892284,
 1.0386341019613787, 1.0388684794110492, 1.0391029097501721, 1.0393373929906822,
 1.0395719291445176, 1.0398065182236185, 1.0400411602399278, 1.0402758552053915,
 1.0405106031319582, 1.0407454040315787, 1.0409802579162071, 1.0412151647977996,
 1.0414501246883161, 1.0416851375997183, 1.0419202035439705, 1.0421553225330404,
 1.042390494578898, 1.042625719693516, 1.0428609978888699, 1.043096329176938,
 1.0433317135697009, 1.0435671510791424, 1.0438026417172486, 1.0440381854960086,
 1.0442737824274138, 1.044509432523459, 1.044745135796141, 1.0449808922574599,
 1.0452167019194181, 1.0454525647940205, 1.0456884808932754, 1.0459244502291931,
 1.0461604728137874, 1.0463965486590741, 1.046632677777072, 1.0468688601798024,
 1.0471050958792898, 1.047341384887561, 1.0475777272166455, 1.047814122878576,
 1.048050571885387, 1.0482870742491166, 1.0485236299818055, 1.0487602390954964,
 1.0489969016022356, 1.0492336175140715, 1.0494703868430555, 1.0497072096012419,
 1.0499440858006872, 1.0501810154534512, 1.050417998571596, 1.0506550351671864,
 1.0508921252522903, 1.0511292688389782, 1.0513664659393229, 1.0516037165654004,
 1.0518410207292894, 1.0520783784430709, 1.0523157897188296, 1.0525532545686513,
 1.0527907730046264, 1.0530283450388465, 1.0532659706834067, 1.0535036499504049,
 1.0537413828519411, 1.0539791694001188, 1.0542170096070436, 1.0544549034848243,
 1.0546928510455722, 1.0549308523014012, 1.0551689072644284, 1.0554070159467728,
 1.0556451783605572, 1.0558833945179062, 1.0561216644309479, 1.0563599881118126,
 1.0565983655726334, 1.0568367968255465, 1.0570752818826903, 1.0573138207562065,
 1.057552413458239, 1.0577910600009348, 1.0580297603964437, 1.058268514656918,
 1.0585073227945128, 1.0587461848213857, 1.058985100749698, 1.0592240705916123
};

const double bend_coarse[128] = {
 1, 1.0594630943592953, 1.122462048309373, 1.189207115002721,
 1.2599210498948732, 1.3348398541700344, 1.4142135623730951, 1.4983070768766815,
 1.5874010519681994, 1.681792830507429, 1.7817974362806785, 1.8877486253633868,
 2, 2.1189261887185906, 2.244924096618746, 2.3784142300054421,
 2.5198420997897464, 2.6696797083400687, 2.8284271247461903, 2.996614153753363,
 3.1748021039363992, 3.363585661014858, 3.5635948725613571, 3.7754972507267741,
 4, 4.2378523774371812, 4.4898481932374912, 4.7568284600108841,
 5.0396841995794928, 5.3393594166801366, 5.6568542494923806, 5.993228307506727,
 6.3496042078727974, 6.727171322029716, 7.1271897451227151, 7.5509945014535473,
 8, 8.4757047548743625, 8.9796963864749824, 9.5136569200217682,
 10.079368399158986, 10.678718833360273, 11.313708498984761, 11.986456615013454,
 12.699208415745595, 13.454342644059432, 14.25437949024543, 15.101989002907095,
 16, 16.951409509748721, 17.959392772949972, 19.027313840043536,
 20.158736798317967, 21.357437666720553, 22.627416997969522, 23.972913230026901,
 25.398416831491197, 26.908685288118864, 28.508758980490853, 30.203978005814196,
 32, 33.902819019497443, 35.918785545899944, 38.054627680087073,
 40.317473596635935, 42.714875333441107, 45.254833995939045, 47.945826460053802,
 50.796833662982394, 53.817370576237728, 57.017517960981706, 60.407956011628393,
 64, 67.805638038994886, 71.837571091799887, 76.109255360174146,
 80.63494719327187, 85.429750666882214, 90.509667991878089, 95.891652920107603,
 101.59366732596479, 107.63474115247546, 114.03503592196341, 120.81591202325679,
 128, 135.61127607798977, 143.67514218359977, 152.21851072034829,
 161.26989438654374, 170.85950133376443, 181.01933598375618, 191.78330584021521,
 203.18733465192958, 215.26948230495091, 228.07007184392683, 241.63182404651357,
 256, 271.22255215597971, 287.35028436719938, 304.43702144069658,
 322.53978877308765, 341.71900266752868, 362.03867196751236, 383.56661168043064,
 406.37466930385892, 430.53896460990183, 456.14014368785394, 483.26364809302686,
 512, 542.44510431195943, 574.70056873439876, 608.87404288139317,
 645.0795775461753, 683.43800533505737, 724.07734393502471, 767.13322336086128,
 812.74933860771785, 861.07792921980365, 912.28028737570787, 966.52729618605372,
 1024, 1084.8902086239189, 1149.4011374687975, 1217.7480857627863,
 1290.1591550923506, 1366.8760106701147, 1448.1546878700494, 1534.2664467217226
};
