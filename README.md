# ofxMotive

## Introduction

This addon allows for easy access to Optitrack Motive's API for accessing 2D and 3D optical motion capture data

### Systems

Since Motive only works in Windows, this addon has only been developed on Windows 10 with Visual Studios 2019.

### Dependencies

1. [Motive API](https://optitrack.com/downloads/motive.html) (v2.2.0)

#### Installation

1. Ensure that a valid license for Motive exists in the folder `C:\ProgramData\OptiTrack\License`. 

   *Note: It is recommended that users install Motive to their computer in order to validate their license and generate accurate calibration files. However, Motive does not need to be installed for this addon to work, since all Motive v2.2.0 headers, libs and dlls have already been included in the folder `motive`.* By default, Motive installs to the directory `C:\Program Files\OptiTrack\Motive`.

## How to use this addon with your project

1. Add this addon to your addons file.
2. Generate your OF project with the Project Generator.
3. Include the `ofxMotive.props` property sheet. You can do this by opening the project in Visual Studios, navigating to the *Property Manager* window (under *View > Other Windows > Property Manager*), right clicking on your project and selecting *Add Existing Property Sheet*. Select the file *ofxMotive.props* included in this repo.
4. Add your Motive calibration (`.cal`) and profile (`.motive`) files to your project's `bin/data` folder. By default, ofxMotive looks for `calibration.cal` and `profile.motive`. If your files are named differently, you can either rename to match the default, or change the paths with ofxRemoteUI.

## Troubleshooting

##### Poco Include Errors

Sometimes, Project Generator includes the Poco addon's libraries, instead of OpenFramework's libraries. Here's how to fix it:

1. In Project Properties > Configuration Properties > C/C++  > Additional Include Directories, include:
   - `$(OF_ROOT)\libs\poco\include`
2. In Project Properties > Configuration Properties > Linker > Additional Include Directories, include:
   1. `$(OF_ROOT)\libs\poco\lib\vs`
   2. `$(OF_ROOT)\libs\poco\lib\vs\x64\Debug`
   3. `$(OF_ROOT)\libs\poco\lib\vs\x64\Release`
   4. `$(OF_ROOT)\libs\poco\lib\vs\Win32\Debug`
   5. `$(OF_ROOT)\libs\poco\lib\vs\Win32\Release`

##### Cannot Find NPTrackingToolsx64.lib

Motive requires that a number of dll's and lib's are copied into the bin folder, alongside your executable. A Post-Build Step in the ofxMotive.props file should complete this task. However, if after building, you notice that this lib and the others below are not in your bin folder, then complete these steps manually:

1. Copy all files from the folder *motive > bin* into your project's executable directory (*bin*). The files that will be copied include those in the below image. It is crucial for these files to be accessible at the same file structure level as the executable itself. For more information, [see this page](https://v21.wiki.optitrack.com/index.php?title=Motive_API:_Quick_Start_Guide#Library_Files).

![](https://v21.wiki.optitrack.com/images/6/6a/MotiveAPI_RequiredLIB.png)

##### Even with an eSync, cameras are not all in sync

Make sure all of your cables are connected. The ethernet cable from your computer to your network switch should be connected on the switch's 10G port (as opposed to a slower 1G port).

The network switch should have the StormControl feature disabled. If needed, check your switch settings to make sure it is disabled. If not, it will delay network traffic to prevent "storms" of messages. 

## Reference

Useful reference materials include the wikis for both API's:

- Motive API [wiki](https://v20.wiki.optitrack.com/index.php?title=Motive_API)
- Camera SDK [wiki](https://v20.wiki.optitrack.com/index.php?title=Camera_SDK)



