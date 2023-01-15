
# Convert a human-readable Android version (e.g. 13, 6.0.1, 4.4) into an API level.
get_api_level () {
	version_full=$1

	major=$(echo $version_full | cut -d . -f 1 )
	minor=$(echo $version_full | cut -d . -f 2 )
	patch=$(echo $version_full | cut -d . -f 3 )

	if [[ $major == 1 ]]; then
		if [[ $minor == 0 ]]; then
			level=1
		elif [[ $minor == 1 ]]; then
			level=2
		elif [[ $minor == 5 ]]; then
			level=3
		elif [[ $minor == 6 ]]; then
			level=4
		fi
	elif [[ $major == 2 ]]; then
		if [[ $minor == 0 ]]; then
			if [[ $patch == 1 ]]; then
				level=6
			else
				level=5
			fi
		elif [[ $minor == 1 ]]; then
			level=7
		elif [[ $minor == 2 ]]; then
			level=8
		elif [[ $minor == 3 ]]; then
			if [[ $patch == "" ]]; then
				level=9
			elif [[ $patch == 0 ]]; then
				level=9
			elif [[ $patch == 1 ]]; then
				level=9
			elif [[ $patch == 2 ]]; then
				level=9
			elif [[ $patch == 3 ]]; then
				level=10
			elif [[ $patch == 4 ]]; then
				level=10
			fi
		fi
	elif [[ $major == 3 ]]; then
		if [[ $minor == 0 ]]; then
			level=11
		elif [[ $minor == 1 ]]; then
			level=12
		elif [[ $minor == 2 ]]; then
			level=13
		fi
	elif [[ $major == 4 ]]; then
		if [[ $minor == 0 ]]; then
			if [[ $patch == "" ]]; then
				level=14
			elif [[ $patch == 0 ]]; then
				level=14
			elif [[ $patch == 1 ]]; then
				level=14
			elif [[ $patch == 2 ]]; then
				level=14
			elif [[ $patch == 3 ]]; then
				level=15
			elif [[ $patch == 4 ]]; then
				level=15
			fi
		elif [[ $minor == 1 ]]; then
			level=16
		elif [[ $minor == 2 ]]; then
			level=17
		elif [[ $minor == 3 ]]; then
			level=18
		elif [[ $minor == 4 ]]; then
			level=19
		fi
	# API_LEVEL 20 corresponds to Android 4.4W ==  which isn't relevant to us
	elif [[ $major == 5 ]]; then
		if [[ $minor == 0 ]]; then
			level=21
		elif [[ $minor == 1 ]]; then
			level=22
		fi
	elif [[ $major == 6 ]]; then
		level=23
	elif [[ $major == 7 ]]; then
		if [[ $minor == 0 ]]; then
			level=24
		elif [[ $minor == 1 ]]; then
			level=25
		fi
	elif [[ $major == 8 ]]; then
		if [[ $minor == 0 ]]; then
			level=26
		elif [[ $minor == 1 ]]; then
			level=27
		fi
	elif [[ $major == 9 ]]; then
		level=28
	elif [[ $major == 10 ]]; then
		level=29
	elif [[ $major == 11 ]]; then
		level=30
	elif [[ $major == 12 ]]; then
	# Android 12 uses both API_LEVEL 31 and 32
	# Default to 31 but allow user to say "12.1" to get 32
		if [[ $minor == 1 ]]; then
			level=32
		else
			level=31
		fi
	elif [[ $major == 13 ]]; then
		level=33
	fi

	echo $level
}

# Configure the LDSP build system to build for the given phone model, Android version, and project path.
configure () {
  # path to hardware config
  hw_config="./phones/$VENDOR/$MODEL/ldsp_hw_config.json"

  # target ABI
  arch=$(grep 'target architecture' "$hw_config" | cut -d \" -f 4)
  if [[ $arch == "armv7a" ]]; then
    abi="armeabi-v7a"
  elif [[ $arch == "aarch64" ]]; then
    abi="arm64-v8a"
  elif [[ $arch == "x86" ]]; then
    abi="x86"
  elif [[ $arch == "x86_64" ]]; then
    abi="x86_64"
  else
    echo "Unknown architecture: $arch"
    exit 1
  fi

  # target Android version
  api_version=$(get_api_level "$VERSION")

  # support for NEON floating-point unit
  neon_setting=$(grep 'supports neon floating point unit' "$hw_config" | cut -d \" -f 4)
  if [[ $arch == "armv7a" ]];
  then
    if [[ $neon_setting =~ ^(true|True|yes|Yes|1)$ ]];
    then
      neon="-DANDROID_ARM_NEON=ON"
    else
      neon="-DANDROID_ARM_NEON=OFF"
    fi
  else
    neon=""
  fi


  cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$api_version "-DANDROID_NDK=$NDK" $neon "-DLDSP_PROJECT=$PROJECT" -G Ninja .
}

# Build the user project.
build () {
  ninja
}

# Push the user project and LDSP hardware config to the phone.
push () {
  hw_config="./phones/$VENDOR/$MODEL/ldsp_hw_config.json"

  adb root
  adb shell "mkdir -p /data/ldsp"
	adb push "$hw_config" /data/ldsp/ldsp_hw_config.json
	adb push bin/ldsp   /data/ldsp/ldsp
}

# Push the user project and LDSP hardware config to the phone's SD card.
push_sdcard () {
  hw_config="./phones/$VENDOR/$MODEL/ldsp_hw_config.json"

  adb root
  adb shell "mkdir -p /sdcard/ldsp"
	adb push "$hw_config" /sdcard/ldsp/ldsp_hw_config.json
	adb push bin/ldsp   /sdcard/ldsp/ldsp
}

# Run the user project on the phone.
run () {
  adb shell "cd /data/ldsp/ && ./ldsp $@"
}

STEPS=()

while [[ $# -gt 0 ]]; do
  case $1 in
    --vendor=*)
      VENDOR="${1#*=}"
      shift
      ;;
    --vendor|-v)
      VENDOR="$2"
      shift
      shift
      ;;
    --model=*)
      MODEL="${1#*=}"
      shift
      ;;
    --model|-m)
      MODEL="$2"
      shift
      shift
      ;;
    --project=*)
      PROJECT="${1#*=}"
      shift
      ;;
    --project|-p)
      PROJECT="$2"
      shift
      shift
      ;;
    --version=*)
      VERSION="${1#*=}"
      shift
      ;;
    --version|-a)
      VERSION="$2"
      shift
      shift
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    run)
      # any arguments passed to "run" are passed to the user project
      STEPS+=("run")
      break
      ;;
    *)
      STEPS+=("$1")
      shift
      ;;
  esac
done

if [ ${#STEPS[@]} -eq 0 ] ; then
  echo "ldsp: no steps specified"
  echo "usage: ldsp [options] [steps] [run args]"
  echo "options:"
  echo "  --vendor=VENDOR, -v VENDOR\tThe vendor of the phone to build for."
  echo "  --model=MODEL, -m MODEL\tThe model of the phone to build for."
  echo "  --project=PROJECT, -p PROJECT\tThe path to the user project to build."
  echo "  --version=VERSION, -a VERSION\tThe Android version to build for."
  echo "steps:"
  echo "  configure\t\t\tConfigure the LDSP build system for the specified phone, version, and project."
  echo "  build\t\t\t\tBuild the user project."
  echo "  push\t\t\t\tPush the user project and LDSP hardware configuration to the phone."
  echo "  push_sdcard\t\t\tPush the user project and LDSP hardware configuration to the phone's SD card."
  echo "  run\t\t\t\tRun the user project on the phone."
  echo "  \t\t\t\t(Any arguments passed after \"run\" are passed to the user project.)"
fi

for i in "${STEPS[@]}"; do
  case $i in
    configure)
      configure
      ;;
    build)
      build
      ;;
    push)
      push
      ;;
    push_sdcard)
      push_sdcard
      ;;
    run)
      run "${@:2}"
      ;;
    *)
      echo "Unknown step $i"
      exit 1
      ;;
  esac
done
