ifeq ($(ANDROID_MAJOR),1)
	ifeq ($(ANRDOID_MINOR),0)
		API_LEVEL := 1
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 2
	else ifeq ($(ANDROID_MINOR),5)
		API_LEVEL := 3
	else ifeq ($(ANDROID_MINOR),6)
		API_LEVEL := 4
	endif
else ifeq ($(ANDROID_MAJOR),2)
	ifeq ($(ANDROID_MINOR),0)
		ifeq ($(ANDROID_PATCH),1)
			API_LEVEL := 6
		else
			API_LEVEL := 5
		endif
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 7
	else ifeq ($(ANDROID_MINOR),2)
		API_LEVEL := 8
	else ifeq ($(ANDROID_MINOR),3)
		ifeq ($(ANDROID_PATCH),)
			API_LEVEL := 9
		else ifeq ($(ANDROID_PATCH),0)
			API_LEVEL := 9
		else ifeq ($(ANDROID_PATCH),1)
			API_LEVEL := 9
		else ifeq ($(ANDROID_PATCH),2)
			API_LEVEL := 9
		else ifeq ($(ANDROID_PATCH),3)
			API_LEVEL := 10
		else ifeq ($(ANDROID_PATCH),4)
			API_LEVEL := 10
		endif
	endif
else ifeq ($(ANDROID_MAJOR),3)
	ifeq ($(ANDROID_MINOR),0)
		API_LEVEL := 11
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 12
	else ifeq ($(ANDROID_MINOR),2)
		API_LEVEL := 13
	endif
else ifeq ($(ANDROID_MAJOR),4)
	ifeq ($(ANDROID_MINOR),0)
		ifeq ($(ANDROID_PATCH),)
			API_LEVEL := 14
		else ifeq ($(ANDROID_PATCH),0)
			API_LEVEL := 14
		else ifeq ($(ANDROID_PATCH),1)
			API_LEVEL := 14
		else ifeq ($(ANDROID_PATCH),2)
			API_LEVEL := 14
		else ifeq ($(ANDROID_PATCH),3)
			API_LEVEL := 15
		else ifeq ($(ANDROID_PATCH),4)
			API_LEVEL := 15
		endif
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 16
	else ifeq ($(ANDROID_MINOR),2)
		API_LEVEL := 17
	else ifeq ($(ANDROID_MINOR),3)
		API_LEVEL := 18
	else ifeq ($(ANDROID_MINOR),4)
		API_LEVEL := 19
# API_LEVEL 20 corresponds to Android 4.4W, which isn't relevant to us
	endif
else ifeq ($(ANDROID_MAJOR),5)
	ifeq ($(ANDROID_MINOR),0)
		API_LEVEL := 21
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 22
	endif
else ifeq ($(ANDROID_MAJOR),6)
	API_LEVEL := 23
else ifeq ($(ANDROID_MAJOR),7)
	ifeq ($(ANDROID_MINOR),0)
		API_LEVEL := 24
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 25
	endif
else ifeq ($(ANDROID_MAJOR),8)
	ifeq ($(ANDROID_MINOR),0)
		API_LEVEL := 26
	else ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 27
	endif
else ifeq ($(ANDROID_MAJOR),9)
	API_LEVEL := 28
else ifeq ($(ANDROID_MAJOR),10)
	API_LEVEL := 29
else ifeq ($(ANDROID_MAJOR),11)
	API_LEVEL := 30
else ifeq ($(ANDROID_MAJOR),12)
# Android 12 uses both API_LEVEL 31 and 32
# Default to 31, but allow user to say "12.1" to get 32
	ifeq ($(ANDROID_MINOR),1)
		API_LEVEL := 32
	else
		API_LEVEL := 31
	endif
else ifeq ($(ANDROID_MAJOR),13)
	API_LEVEL := 33
endif
