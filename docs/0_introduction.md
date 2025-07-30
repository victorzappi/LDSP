# INTRODUCTION
## BEFORE YOU START!
Your Android Phone needs to be **rooted**. This process highly depends on the specific brand and model of your phone, and may take from 5 minutes to several days. Notably, some vendors permanently lock down the bootlader of certain phones, making rooting not possible. The XDA forum is the best place to look for guides/how-tos and get help:
[https://forum.xda-developers.com/](https://forum.xda-developers.com/)

Once the phone is rooted, make sure that you have *Developer Options* enabled on the phone and inside that manu activate *USB debugging*.

### Will It Work With My Phone?
In theory, any phone running Android 2.3 or later can be hacked and run LDSP. In practice, some devices—and especially certain vendor-modified builds—are far more difficult to deal with, and a few depart so much from the Android blueprint provided by Google that using LDSP resuls impossible. More on this stuff is explained in one of the later doc pages ([Phone Configuration](3_phone_config.md)).

**Here is a list of the phones we tested**:
- Asus ZenFone 2 Laser (ZE500KL - Z00ED)
- Asus ZenFone Go (ZB500KG - X00BD)
- Huawei P8 Lite (alice)
- Huawei P10 (VTR-L09)
- Huawei P10 Lite (WAS-LX1A)
- LG G2 Mini (g2m)
- LG Nexus 4 (mako)
- LG Nexus 5 (bullhead)
- Motorola G7 (river)
- Samsung Galaxy Tab 4 (SM-T237P)
- Xiaomi Mi9 Lite/CC9 (pyxis)
- Xiaomi Mi8 Lite (platina)

Each comes with a pre-made configuration file, stored in [LDSP/phones](../phones) and orgnaized by vendor.
If you configure a new phone, please let us know and we will add the configuration file you created for it! 


[Next: Dependencies](1_dependencies.md)