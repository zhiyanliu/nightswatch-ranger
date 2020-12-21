## What is this

This is the code repository of an IoT device [OTA](https://en.wikipedia.org/wiki/Over-the-air_programming) agent running on the AWS IoT Core service managed device. The code name of this component is [Night's Watch](https://gameofthrones.fandom.com/wiki/Night%27s_Watch) - [Ranger](https://gameofthrones.fandom.com/wiki/Ranger), which is the part of Night's Watch project.

Mainly, and [currently](http://git.awsrun.com/rp/nightswatch-ranger/blob/master/job_op_types.c), Night's Watch - Ranger provides three functions:

1. Device certificates OTA.
2. Agent itself OTA.
3. Managed application deployment and OTA. Support containerized and non-containerized deployment, upgrade and destroy for user-defined application package.

## Why [we](mailto:awscn-sa-prototyping@amazon.com) develop it

AWS IoT Core service provides a set of great features, as the fundamental and necessary mechanism customer can easily build own business logic base on it. However beside the core and basic part, some common and high level paradigm is missing and need to be developed by the customer, again and again, device OTA just is the one which we realized in prototyping daily works.

To prevent our SA and customer to re-develop this kind of OTA agent for AWS IoT Core managed device again, we polished our OTA project to make it as general as possible for common usage, to accelerate the PoC, prototyping case and AWS IoT related service delivery.

>> **Note:**
>>
>> This project is truly under continuative develop stage, we'd like to collect the feedback and include the enhancement in follow-up release to share them with all users. 
>>
>> **DISCLAIMER: This project is NOT intended for a production environment, and USE AT OWN RISK!**  

## Limit

This project does not work for you if:

* Your device will not manged by AWS IoT Core service.
* You do not have cross compiling toolchain of the device vendor but want to compile and run this OTA agent on your device.
* You would not like to involve C lang (and AWS IoT Core embedded C SDK) to your technical stack.

## How to build

### Setup Environment

As a C lang project, it needs Build tool and GCC toolchain to compile and link target. To easy development, `Makefile` currently supports Mac and Ubuntu platforms, you can coding and debug on local MBP, and integration on remote Ubuntu/Linux host. QuecOpen Linux cross compiling toolchain is supported as well on Ubuntu platform.

- `Mac platform`:

  ```
  xcode-select --install
  /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  brew install openssl # openssl/md5.h
  ```

- `Ubuntu platform`:

  ```
  sudo apt install build-essential
  sudo apt install libssl-dev # openssl/md5.h
  ```

- `QuecOpen Linux`: execute follow commands, and read official document `Quectel_EC2x&AG35-QuecOpen`.

  ```
  tar jxf EC20CEFAGR06A05M4G_OCPU_SDK.tar.bz2
  cd ql-ol-sdk/ql-ol-crosstool
  source ql-ol-crosstool-env-init
  ```

>> **Note:**
>>
>> When you install and setup QuecOpen Linux SDK by executing the command `source ql-ol-crosstool-env-init` at first time, you might meet follow issue, read troubleshooting section to get the solution as the reference.
>>
>> ```
>> sed: can't read ./relocate_sdk.py: No such file or directory
>> ```
>>
>> After the installation and setup, you can use use `arm-oe-linux-gnueabi-gcc -v` command to verify if cross complie toolchain is ready to build the project, you might meet below 2 kinds of issue, read troubleshooting section to get the solution as the reference.
>>
>> ```
>> Relocating .../ql-ol-sdk/ql-ol-crosstool/sysroots/x86_64-oesdk-linux/usr/bin/python: line 4:
>> /ql-ol-sdk/ql-ol-crosstool/sysroots/x86_64-oesdk-linux/usr/bin/python2.7.real: No such file or directory
>> SDK could not be set up. Relocate script failed. Abort!
>> ```
>>
>> ```
>> ql-ol-crosstool/sysroots/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-gcc:
>> No such file or directory
>> ```

### Basic

1. ``git clone git@git.awsrun.com:rp/nightswatch-ranger.git`` or ``git clone http://git.awsrun.com/rp/nightswatch-ranger.git``
2. ``cd nightswatch-ranger``
3. ``source <QuecOpen Linux SDK>/ql-ol-crosstool/ql-ol-crosstool-env-init``, do this for cross compiling for QuecOpen Linux platform only, and execute this for each new session, environment varibables are exported and dedicated.
4. ``make``, `nightswatch-ranger` and `nightswatch-rund` binaries are built out to current directory if build process is executed successfully.
5. ``make install``, Install compiled binaries to their running directory.

### Advanced

Build supports follow 7 targets currently, e.g. you can execute `make libawsiot.a` to build AWS IoT SDK library only.

1. `aws-iot-device-sdk-embedded-C` Get AWS IoT SDK vendor package.
2. `libawsiot.a` Compile AWS IoT SDK static library.
3. `nightswatch-ranger` Compile and link Night's Watch Ranger binary.
4. `nightswatch-rund` Compile and link Night's Watch application launcher binary.
5. `all` Currently it does `nightswatch-ranger` and `nightswatch-rund` targets. Default target.
6. `install` Install Ranger and the launcher binaries to bin directory with dual partitions structure of certificate OTA, Ranger OTA and apps directory.
7. `vendor_clean` Remove local AWS IoT SDK vendor package and compiled static library.
8. `clean` Remove all output files of last build, prepare for next complete and clean Night's Watch Ranger build.

>>**Note:**
>>
>> libawsiot.a static library works for current project and client only due to it contains your special configuration e.g. IoT MQTT server address and port, client id for the device. So don't copy this file to other embedded C project or your different device. From engineering perspective, this library is used to separate common AWS IoT SDK from the OTA agent business logic as an upstream vendor package, as the result, you can do `make aws-iot-device-sdk-embedded-C` to get vendor code outside this OTA agent project repository and keep it up-to-date.
>>
>> In future, you can extract and parameterize these configurations out of the library, however it means you need to change AWS IoT SDK code or enhance Build logic.

### Deployment directory structure

```
<NIGHTS_WATCH_RANGER_HOME_DIR>
├── [lrwxrwxrwx]  ranger -> bin/p1/nightswatch-ranger
├── [drwxr-xr-x]  apps
│   ├── [-rw-rw-r--]  config.json.runc.tpl
│   ├── [-rw-rw-r--]  config.json.rund.tpl
│   ├── [-rwxrwxr-x]  runc
│   └── [-rwxr-xr-x]  rund
├── [drwxr-xr-x]  bin
│   ├── [drwxr-xr-x]  p1
│   │   └── [-rwxr-xr-x]  nightswatch-ranger
│   └── [drwxrwxr-x]  p2
├── [drwxr-xr-x]  certs
│   ├── [lrwxrwxrwx]  latest -> ./p1
│   ├── [drwxr-xr-x]  p1
│   │   ├── [-rw-r--r--]  cert.pem
│   │   ├── [-rw-r--r--]  private.key
│   │   ├── [-rw-r--r--]  public.key
│   │   └── [-rw-r--r--]  root-ca.crt
│   └── [drwxr-xr-x]  p2
└── [drwxrwxr-x]  funcs
    └── [drwxrwxr-x]  router
```

## How to config

Overall you need to config Night's Watch Ranger from below two sides:

1. Setup customer specific device client parameter configuration.
2. Place certificate, public and private key files to project home directory.

### Place certificate, public and private key files

The client needs to use valid certificate, public and private key files to establish authorized communication between the device client and AWS IoT Core service.

When initialize setup, you can retrieve these key files from AWS IoT Core server by web Console explained by the [document](https://docs.aws.amazon.com/iot/latest/developerguide/create-device-certificate.html), then save them to `certs/p1` directory under project home. The file name of certificate, public and private key should keep consistent with the configuration you provided in the parameter configurations mentioned in follow section, by default the file names are:

- `root-ca.crt`: root CA file name.
- `cert.pem`: device signed certificate file name.
- `private.key`: device private key file name.

### Device client parameter configuration

Such parameters of client code running in the device are configurable. Some ones are important, others are optional and default values are provied for general case.

The complete configurations are listed in `aws_iot_config.h` file. You need to take care important ones mentioned below:

>>**Note:**
>>
>>You need to finish this configuration before to build the project, due to currently we use [`#define` preprocessor directive](https://www.techonthenet.com/c_language/constants/create_define.php) to provide this kind of config value. It means the configuration will be built into the binary statically and can not be changed durning runtime.

- `AWS_IOT_MQTT_HOST`: the customer specific MQTT HOST. The same will be used for Thing Shadow.
- `AWS_IOT_MQTT_CLIENT_ID`: the MQTT client ID should be unique for every device.
- `AWS_IOT_MY_THING_NAME`: the thing Name of the Shadow this device is associated with.
- `AWS_IOT_ROOT_CA_FILENAME`: root CA file name, equals to the real file name you placed in `certs/p1` directory.
- `AWS_IOT_CERTIFICATE_FILENAME`: device signed certificate file name, equals to the real file name you placed in `certs/p1` directory.
- `AWS_IOT_PRIVATE_KEY_FILENAME`: device private key file name, equals to the real file name you placed in `certs/p1` directory.

Other specific configurations are listed as well for different service, e.g. MQTT PubSub, Thing Shadow. You might follow the inline comments and the directive name of the configuration is straightforward.

### Using HTTPS proxy

Device needs to download package from S3 service, to resolve such network issue, HTTPS proxy is supported. You can export HTTPS_PROXY environment to tell which proxy endpoint is used during the download as usual. For example:

```
export http_proxy=http://127.0.0.1:1087
export https_proxy=$http_proxy
```
### Dependencies

- `unzip`: If you need Night's Watch Ranger supports Certificate and Agent OTA operation. Install it on Ubuntu by ``apt-get install unzip``.
- `tar`: If you need Night's Watch Ranger supports Application Deployment operation.
- `runc`: If you need Night's Watch Ranger supports Application Deployment operation with container mode. Refer RUNC at [here](https://github.com/opencontainers/runc).

## Troubleshooting

>> **Note:**
>> This is the section to try to assist builder resolves issues during the development, read this as the reference and take the actions according to your own environment.

### sed: can't read ./relocate_sdk.py: No such file or directory

1. When you first time to install and setup QuecOpen Linux SDK, make sure you are execute `source ql-ol-crosstool-env-init` in the directory `<QuecOpen Linux SDK>/ql-ol-sdk/ql-ol-crosstool`.
2. The installation and set needs python2 supports, in Ubuntu install it by command `sudo apt install python-minimal`.

### arm-oe-linux-gnueabi-gcc: No such file or directory

When command `arm-oe-linux-gnueabi-gcc -v` failed and output `ql-ol-crosstool/sysroots/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-gcc: No such file or directory`, it might caused by shared object dependencies missing.

Run `ldd ql-ol-crosstool/sysroots/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-gcc`, to find out if any shared object dependencies is missing.

For example, libraray `/home/eve/Eve_Linux_Server/Qualcomm/MDM9x07/OpenLinux/LE.1.0.c3/Release/EC20CEFAG/R06A05/apps_proc/oe-core/build/tmp-glibc/deploy/sdk/ql-ol-sdk/ql-ol-crosstool/sysroots/x86_64-oesdk-linux/lib/ld-linux-x86-64.so.2` is not existing really in my development host, even directory `/home/eve` is missing. The solution is simple to create a link file at `/home/eve/Eve_Linux_Server/Qualcomm/MDM9x07/OpenLinux/LE.1.0.c3/Release/EC20CEFAG/R06A05/apps_proc/oe-core/build/tmp-glibc/deploy/sdk` and point to valid QuecOpen Linux SDK directory.

Kindly reminder, don't forget to give enough read permission to allow current user to access the files and directories under `/home/eve`.

### python2.7.real: No such file or directory

Same reason as above one. Any `No such file or directory` issue but the file you executed is actually exising, can use ``ldd`` to check if any shared object dependencies of the linked/ELF file are missing.

## Key TODO plan:

- [ ] Explicitly re-subscribe IoT job topics after MQTT client re-connect to the IoT Core service.
- [ ] Lock MQTT client to prevent MQTT_UNEXPECTED_CLIENT_STATE_ERROR. 
- [ ] Allow agent startup without Internet.

## Contributor

* Zhi Yan Liu, [liuzhiya@amazon.com](mailto:liuzhiya@amazon.com)
* You. Welcome any feedback and issue report, further more, idea and code contribution are highly encouraged.
