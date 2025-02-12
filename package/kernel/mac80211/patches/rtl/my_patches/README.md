### My Patches ###

Try to enable SDIO STA mode, the chips failed on AP mode:

* WiFi AP does not work with auto channel.
* My PC cannot connect to WiFi AP. 

```log
[   31.641470] br-lan: port 2(phy0-ap0) entered blocking state
[   31.647084] br-lan: port 2(phy0-ap0) entered forwarding state
[  240.626731] rtw_8821cs mmc2:0001:1: error beacon valid
[  240.632007] rtw_8821cs mmc2:0001:1: failed to download drv rsvd page
[  294.692563] rtw_8821cs mmc2:0001:1: error beacon valid
[  294.697823] rtw_8821cs mmc2:0001:1: failed to download drv rsvd page
[  348.758780] rtw_8821cs mmc2:0001:1: error beacon valid
[  348.764040] rtw_8821cs mmc2:0001:1: failed to download drv rsvd page
[  348.770426] rtw_8821cs mmc2:0001:1: failed to download beacon
[  792.973082] rtw_8821cs mmc2:0001:1: error beacon valid
[  792.978526] rtw_8821cs mmc2:0001:1: failed to download drv rsvd page
[ 1189.160623] rtw_8821cs mmc2:0001:1: error beacon valid
[ 1189.165884] rtw_8821cs mmc2:0001:1: failed to download drv rsvd page
```


