This is the repository of the AWS RP project for iRootech DMP agent running in the device.

## How to build

### Setup Environment

As a C lang project, it needs Build tool and GCC toolchain to compile and link target. To easy development, `Makefile` supports Mac and Ubuntu platforms, you can coding and debug on local MBP, and integration on remote Ubuntu/Linux host.

- `Mac platform`: ``xcode-select --install``

- `Ubuntu platform`: ``sudo apt install build-essential``

### Basic

1. `git clone ssh://git-codecommit.ap-northeast-1.amazonaws.com/v1/repos/irootech-dmp-rp-agent`
2. Change current directory to project directory.
3. ``make``
4. Binary `dmpagent` is built out to current directory if build process is executed successfully.

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

Overall you need to config DMP agent from below two sides:

1. Setup customer specific device client parameter configuration.
2. Place certificate, public and private key files to project home directory.

### Place certificate, public and private key files

The client need to use valid certificate, public and private key files to establish authorized communication between the device client and AWS IoT Core service.

You can retrieve these key files from AWS IoT Core server by web Console explained by the [document](https://docs.aws.amazon.com/iot/latest/developerguide/create-device-certificate.html), then save them to `certs` directory under project home. The file name of certificate, public and private key should keep consistent with the configuration you provided in the parameter configurations mentioned in follow section, by default the file names are:

- `root-ca.crt`: Root CA file name.
- `cert.pem`: device signed certificate file name.
- `privkey.pem`: device private key file name.

### Device client parameter configuration

Such parameters of client code running in the device are configurable. Some ones are important, others are optional and default values are provied for general case.

The complete configurations are listed in `aws_iot_config.h` file. You need to take care important ones mentioned below:

>>**Note:**
>>
>>You need to finish this configuration before to build the project, due to currently we use [`#define` preprocessor directive](https://www.techonthenet.com/c_language/constants/create_define.php) to provide this kind of config value. It means the configuration will be built into the binary statically and can not be changed durning runtime.

- `AWS_IOT_MQTT_HOST`: the customer specific MQTT HOST. The same will be used for Thing Shadow.
- `AWS_IOT_MQTT_CLIENT_ID`: the MQTT client ID should be unique for every device.
- `AWS_IOT_MY_THING_NAME`: the thing Name of the Shadow this device is associated with.
- `AWS_IOT_ROOT_CA_FILENAME`: root CA file name, equals to the real file name you placed in `certs` directory.
- `AWS_IOT_CERTIFICATE_FILENAME`: device signed certificate file name, equals to the real file name you placed in `certs` directory.
- `AWS_IOT_PRIVATE_KEY_FILENAME`: device private key file name, equals to the real file name you placed in `certs` directory.

Other specific configurations are listed as well for different service, e.g. MQTT PubSub, Thing Shadow. You might follow the inline comments and the directive name of the configuration is straightforward.
