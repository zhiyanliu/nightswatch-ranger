This is the repository of the AWS RP project for iRootech DMP agent running in the device.

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

- `QuecOpen Linux`: execute follow commands, and read offical document `Quectel_EC2x&AG35-QuecOpen`.

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

1. `git clone ssh://git-codecommit.ap-northeast-1.amazonaws.com/v1/repos/irootech-dmp-rp-agent`
2. `cd irootech-dmp-rp-agent`
3. ``source <QuecOpen Linux SDK>/ql-ol-crosstool/ql-ol-crosstool-env-init``, do this for cross compiling for QuecOpen Linux platform only, and execute this for each new session, environment varibables are exported and dedicated.
4. ``make``, and inary `dmpagent` is built out to current directory if build process is executed successfully.

#### Advanced

Build supports follow 6 targets currently, e.g. you can execute `make libawsiot.a` to build AWS IoT SDK library only.

1. `aws-iot-device-sdk-embedded-C` Get AWS IoT SDK vendor package.
2. `libawsiot.a` Compile AWS IoT SDK static library.
3. `dmpagent` Compile and link DMP agent binary.
4. `all` Currently it does `dmpagent` target only.
5. `vendor_clean` Remove local AWS IoT SDK vendor package and compiled static library.
6. `clean` Remove all output files of last build, prepare for next complete and clean DMP agent build.

>>**Note:**
>>
>> libawsiot.a static library works for current project and client only due to it contains your special configuration e.g. IoT MQTT server address and port, client id for the device. So don't copy this file to other embedded C project or your different device. From engineering perspective, this libraray is used to separate common AWS IoT SDK from the DMP agent business logic as an upstream vendor package, as the result, you can do `make aws-iot-device-sdk-embedded-C` to get vendor code outside this DMP agent project repository and keep it up-to-date.
>>
>> In future, you can extract and parameterize these configurations out of the libraray, however it means you need to change AWS IoT SDK code or enhance Build logic.

## How to config

Overall you need to config DMP device agent from below two sides:

1. Setup customer specific device client parameter configuration.
2. Place certificate, public and private key files to project home directory.

### Place certificate, public and private key files

The client needs to use valid certificate, public and private key files to establish authorized communication between the device client and AWS IoT Core service.

When initialize setup, you can retrieve these key files from AWS IoT Core server by web Console explained by the [document](https://docs.aws.amazon.com/iot/latest/developerguide/create-device-certificate.html), then save them to `certs/latest` directory under project home. The file name of certificate, public and private key should keep consistent with the configuration you provided in the parameter configurations mentioned in follow section, by default the file names are:

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
- `AWS_IOT_ROOT_CA_FILENAME`: root CA file name, equals to the real file name you placed in `certs/latest` directory.
- `AWS_IOT_CERTIFICATE_FILENAME`: device signed certificate file name, equals to the real file name you placed in `certs/latest` directory.
- `AWS_IOT_PRIVATE_KEY_FILENAME`: device private key file name, equals to the real file name you placed in `certs/latest` directory.

Other specific configurations are listed as well for different service, e.g. MQTT PubSub, Thing Shadow. You might follow the inline comments and the directive name of the configuration is straightforward.

### Using HTTPS proxy

Device needs to download package from S3 service, to resolve such network issue, HTTPS proxy is supported. You can export HTTPS_PROXY environment to tell which proxy endpoint is used during the download as usual. For example:

```
export http_proxy=http://127.0.0.1:1087
export https_proxy=$http_proxy
```
### Dependencies

- `unzip`: install it on Ubuntu by ``apt-get install unzip``.

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
