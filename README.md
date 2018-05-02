# SteganoWeb
Web application that allows end user to perform image and audio steganography
This application is built as a final project submission for UIUC CS 460 : Cyber Security Lab.

## Demo
A demo can be found at this address : http://165.227.49.237:8080/

NOTE: This demo will be taken down at a later point and is being only served as a proof of concept for grading purposes.

## Motivation
Images (PNG, JPG, BMP) and Audio (WAV) files are dense information formats which use significant precision to store their information. The precision is high enough such that even if the lower bits change, the apparent or audible change in the media would be indistiguishable from the orginal for the user. Using this idea, we can encode any secret file into a supported ballast file where the noise introduced by us would by 0.39% in case of WAV files and 0.78% change in case of images, which is hard for most humans to notice. We also encode some meta information during our encoding process such that we don't require the original file to decode our message, allowing for successful one time copies of our data.

## Drawbacks
This method, however subtle can still be detected by Fourier Transform of the media which can reveal our secret information piggy-backing on the main media information. Also, the method used is not robust, and any change to the binary (say compression by social media platforms) will destroy our message. An area of improvement would to increase the robustness of this technique, and better encapsulation such that its harder to detect.

## Usage
The left column provides Encoding interface while the right column provides Decoding interface.
  ### Encoding
  The user first uploads a file called the Ballast which would serve as the medium in which we encode the secret. If the file type is accepted, another form would reveal itself where the user would be allowed to upload the secret file. This can be any file as long as its size is below the allowed size displayed to the user when the upload the ballast. Finally the third form would unravel where when the user commences the encoding process, on successful completion, a download link would appear for the user.
  ### Decoding
  Decoding form allows user to upload either an encoded wav or png file, and if successful provides a link to download the decoded secret binary file.
  
## Installation
```
npm install
make (Create the required wavcodec binary, you'll need gcc for this)
npm start (The application can be accessed at localhost:8080 unless specified by you)
```

## Deployement Notes
Change the host url in 'public/ClientSide.js' from localhost:8080 to your hosting address. Do the same for two links in index.html

Forever is a great npm utility if you want this app to be daemonized

## Supported Types
WAV, PNG, JPG and BMP files are supported as of now. In case of images (PNG, JPG, BMP) every byte of secret is encoded per pixel (we take lower 2 bits each from RGBA channels). As transparency is required, output file is always going to be a PNG file.
NOTE: This application uses JIMP to read PNG, JPG and BMP files and as of 4/29/2018, Jimp has has some issues with maintaining file compression (it automatically increases bit-depth from 8 to 32) and as such the file size is bloated considerably. This is unintended behavior and when the library fixes this issue or I discover an alternative, the encoded files should be comparable in size to the given input images.

For WAV files, a variable number of bits are used to encode the secret depending on the relative size difference between them. In worst case, the size of secret can be almost half the size of input ballast wav file.

## Author

Rishabh Asthana {asthana4@illinois.edu}
