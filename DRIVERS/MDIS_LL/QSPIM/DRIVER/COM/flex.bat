rem USAGE flex Flexfilename-without-extension

 echo #define QSPIM_PLD_VERSION "%1.ttf" >qspim_plddata.h
 ttf2arr %1.ttf __QSPIM_PldData >>qspim_plddata.h


