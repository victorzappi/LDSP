#!/bin/bash
# This script controls configuring the LDSP build system, building a project
# using LDSP, and running the resulting binary on a phone.

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
	# API_LEVEL 20 corresponds to Android 4.4W, which isn't relevant to us.
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

  if [[ $level == "" ]]; then
    exit 1
  fi

	echo $level
}

# Configure the LDSP build system to build for the given phone model, Android version, and project path.
configure () {
  if [[ $VENDOR == "" ]]; then
    echo "Cannot configure: Vendor not specified"
    echo "Please specify a phone vendor with --vendor"
    exit 1
  fi

  if [[ $MODEL == "" ]]; then
    echo "Cannot configure: Model not specified"
    echo "Please specify a phone model with --model"
    exit 1
  fi

  # path to hardware config
  hw_config="./phones/$VENDOR/$MODEL/ldsp_hw_config.json"

  if [[ ! -f "$hw_config" ]]; then
    echo "Cannot configure: Hardware config file not found"
    echo "Please ensure that an ldsp_hw_config.json file exists for \"$VENDOR/$MODEL\""
    exit 1
  fi

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
    echo "Cannot configure: Unknown architecture: $arch"
    exit 1
  fi

  # target Android version
  api_level=$(get_api_level "$VERSION")
  exit_code=$?

  api_define="-DAPI=$api_level"

  if [[ $exit_code != 0 ]]; then
    echo "Cannot configure: Unknown Android version: $version_full"
    exit $exit_code
  fi

  # support for NEON floating-point unit
  explicit_neon="-DEXPLICIT_ARM_NEON=0"
  neon_setting=$(grep 'supports neon floating point unit' "$hw_config" | cut -d \" -f 4)
  if [[ $arch == "armv7a" ]];
  then
    if [[ $neon_setting =~ ^(true|True|yes|Yes|1)$ ]];
    then
      neon="-DANDROID_ARM_NEON=ON"
      explicit_neon="-DEXPLICIT_ARM_NEON=1"
    else
      neon=""
    fi
  else
    neon=""
  fi

  if [[ $PROJECT == "" ]]; then
    echo "Cannot configure: Project path not specified"
    echo "Please specify a project path with --project"
    exit 1
  fi

  if [[ ! -d "$PROJECT" ]]; then
    echo "Cannot configure: Project directory does not exist"
    echo "Please specify a valid project path with --project"
    exit 1
  fi

  if [[ ! -d "$NDK" ]]; then
    echo "Cannot configure: NDK not found"
    echo "Please specify a valid NDK path with"
    echo "    export NDK=<path to NDK>"
    exit 1
  fi


  cmake -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$api_level "-DANDROID_NDK=$NDK" $explicit_neon $neon $api_define "-DLDSP_PROJECT=$PROJECT" -G Ninja .
  exit_code=$?
  if [[ $exit_code != 0 ]]; then
    echo "Cannot configure: CMake failed"
    exit $exit_code
  fi
}

# Build the user project.
build () {
  ninja
  exit_code=$?
  if [[ $exit_code != 0 ]]; then
    echo "Cannot build: Ninja failed"
    exit $exit_code
  fi
}

# Clean the built files.
clean () {
  ninja clean
}

# Install the user project, LDSP hardware config and resources to the phone.
install () {
  if [[ ! -f bin/ldsp ]]; then
    echo "Cannot push: No ldsp executable found. Please run \"ldsp build\" first."
    exit 1
  fi

  hw_config="./phones/$VENDOR/$MODEL/ldsp_hw_config.json"
  proj="$PROJECT"


  # create ldsp folder on sdcard
  adb shell "su -c 'mkdir -p /sdcard/ldsp'"

  if [[ ! -f "$hw_config" ]]; then
    echo "WARNING: Hardware config file not found, skipping..."
  else
    adb push "$hw_config" /sdcard/ldsp/ldsp_hw_config.json
  fi

  #TODO switch to project folders on phone?
  # then change name of bin to project name
  # add remove function to delete project folder from phone

  # Push all project resources, including Pd files in Pd projects, but excluding C/C++, assembly, javascript files and folders that contain those files
  # first folders
  find "$PROJECT"/* -type d ! -exec sh -c 'ls -1q "{}"/*.cpp "{}"/*.c "{}"/*.h "{}"/*.hpp "{}"/*.S "{}"/*.s "{}"/*.js 2>/dev/null | grep -q . || echo "{}"' \; | xargs -I{} adb push {} /sdcard/ldsp/
  # then files
  find "$PROJECT" -maxdepth 1 -type f ! \( -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.hpp" -o -name "*.S" -o -name "*.s" -o -name "*.js" \) -exec adb push {} /sdcard/ldsp/ \;

  # finally the ldsp bin
	adb push bin/ldsp /sdcard/ldsp/ldsp

  adb shell "su -c 'mkdir -p /data/ldsp'" # create ldsp folder
  adb shell "su -c 'cp -r /sdcard/ldsp/* /data/ldsp'" # cp all files from sd card temp folder to ldsp folder
  adb shell "su -c 'chmod 777 /data/ldsp/ldsp'" # add exe flag to ldsp bin

  # check if this project has a sketch.js file
  if [[ -e "$PROJECT/sketch.js" ]]; then
      # if it does, remove the contents of /sdcard/ldsp on the device
      adb shell "su -c 'rm -r /sdcard/ldsp/*'"
      # and copy sketch.js to the sdcard
      adb push "$PROJECT/sketch.js" /sdcard/ldsp/sketch.js
  else
      # if it doesn't, just remove the /sdcard/ldsp directory on the device
      adb shell "su -c 'rm -r /sdcard/ldsp'"
  fi
}

# Install the LDSP scripts on the phone.
install_scripts() {
  adb shell "su -c 'mkdir -p /sdcard/ldsp/scripts'" # create temp folder on sdcard
  adb push ./scripts/ldsp_* /sdcard/ldsp/scripts/ # push scripts there
  adb shell "su -c 'mkdir -p /data/ldsp/scripts'" # create ldsp scripts folder
  adb shell "su -c 'cp /sdcard/ldsp/scripts/* /data/ldsp/scripts'" # copy scripts to ldsp scripts folder
  adb shell "su -c 'rm -r /sdcard/ldsp'" # remove temp folder from sd card
}

# Stop the currently-running user project on the phone.
stop () {
  echo "Stopping LDSP..."
  adb shell "su -c 'sh /data/ldsp/scripts/ldsp_stop.sh'"
}

handle_stop() {
  # Gracefully stop the LDSP process
  stop
  # Wait for the LDSP process and ADB shell to terminate properly
  sleep 1
}

# Run the user project on the phone.
run () {
  # Run adb shell in a subshell, so that it doesn't receive the SIGINT signal
  (
    trap "" INT
    adb shell "su -c 'cd /data/ldsp/ && ./ldsp $@'" # we invoke su before running the bin, needed on some phones: https://stackoverflow.com/a/27274416
  ) &


  # Trap the SIGINT signal in our "outer" shell so that we can gracefully
  # stop the LDSP process
  trap handle_stop INT

  # Wait for the subshell to terminate
  wait
}

# Run the user project on the phone in the background.
run_persistent () {
  cat <<EOF | adb shell > /dev/null 2>&1
  cd /data/ldsp/
  nohup ./ldsp $@ > /dev/null 2>&1 &
  exit
EOF
}

# Print usage information.
help () {
  echo -e "usage: ldsp [options] [steps] [run args]"
  echo -e "options:"
  echo -e "  --vendor=VENDOR, -v VENDOR\tThe vendor of the phone to build for."
  echo -e "  --model=MODEL, -m MODEL\tThe model of the phone to build for."
  echo -e "  --project=PROJECT, -p PROJECT\tThe path to the user project to build."
  echo -e "  --version=VERSION, -a VERSION\tThe Android version to build for."
  echo -e "steps:"
  echo -e "  configure\t\t\tConfigure the LDSP build system for the specified phone, version, and project."
  echo -e "  build\t\t\t\tBuild the user project."
  echo -e "  install\t\t\t\tInstall the user project, LDSP hardware config and resources to the phone."
  echo -e "  run\t\t\t\tRun the user project on the phone."
  echo -e "  \t\t\t\t(Any arguments passed after \"run\" are passed to the user project.)"
  echo -e "  stop\t\t\t\tStop the currently-running user project on the phone."
  echo -e "  install_scripts\t\tInstall the LDSP scripts on the phone."
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
    --help|-h)
      STEPS+=("help")
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
  help
  exit 1
fi

for i in "${STEPS[@]}"; do
  case $i in
    configure)
      configure
      ;;
    build)
      build
      ;;
    clean)
      clean
      ;;
    install)
      install
      ;;
    install_scripts)
      install_scripts
      ;;
    run)
      run "${@:2}"
      ;;
    run_persistent)
      run_persistent "${@:2}"
      ;;
    stop)
      stop
      ;;
    help)
      help
      ;;
    *)
      echo "Unknown step $i"
      exit 1
      ;;
  esac
done
