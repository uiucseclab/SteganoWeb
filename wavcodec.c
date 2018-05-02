/*
Variable data density WAV encoder/decoder
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

//Set debug to 1 to display additional information
int debug = 0;

//WAV subchunk struct
typedef struct wav_subchunk
{
  char RIFF[4];			//Must contain "RIFF"
  int32_t fileSize;		//File size of the current file
  char WAVE[4];			//Must contain "WAVE"
} wav_subchunk;

//fmt subchunk struct
typedef struct fmt_subchunk
{
  char fmt[4];			               //Format Chunk Marker, contains "fmt "
  int32_t formatLength;		         //Length of format
  int16_t formatType;		           //Type of format. 1 is PCM, which is uncompressed
  int16_t numChannels; 		         //1 : MONO, 2 : Stereo
  int32_t sampleRate;		           //Number of samples per second.. 44.1KHZ, 48KHZ etc
  int32_t avgBytes;		             //Average number of bytes/second. (sampleRate * BitsPerSample * numChannels)/8
  int16_t overallBytesPerSample;	 //Holds (BytesPerSample * numChannels)
  int16_t BitsPerSample;		       //Bits per sample :|

} fmt_subchunk;

//Arbitrary subchunk struct, can be used for data subchunk
typedef struct subchunk
{
  char chunkLabel[4];
  int32_t chunkSize;
} subchunk;

//Bookeeping information
int32_t fileSize;
fmt_subchunk* fmt;
subchunk* data;
int fmtflag;
int headerSize;

//Function prototypes
void verifyWavheader(wav_subchunk);
void printBytes(char*, size_t);
int findSubchunk(subchunk, FILE*);
void encode(FILE*, FILE*, FILE*);
void decode(FILE*, FILE*);
void cleanUp();

//MAIN
int main(int argc, char** argv)
{
  //Usage
  if(argc < 4 || argc > 5)
  {
    printf("Usage : ./wavcodec encode <input.wav> <message> <output.wav>\nOR      ./wavcodec decode <input.wav> <output>\n");
    exit(1);
  }

  // Open provided input file (Ballast)
  FILE* input = fopen(argv[2], "r");
  if(input == NULL)
  {
    perror("Error opening input file\n");
    exit(1);
  }

  //Bookeeping information and file check
  wav_subchunk wav;
  fread(&wav, sizeof(wav_subchunk), 1, input);
  verifyWavheader(wav);

  subchunk chunk;
  int ret = 0;
  int count = 0;
  // Find 'data' subchunk
  while(ret != 1)
  {
    fread(&chunk, sizeof(subchunk), 1, input);
    ret = findSubchunk(chunk, input);
    count++;
    if(count == 10)
    break;
  }
  if(count == 10)
  {
    printf("Invalid wav audio format, data/fmt subchunk not found\n");
    cleanUp();
    exit(1);
  }

  // +8 to account for "RIFF" and fileSize variable
  headerSize = wav.fileSize - data->chunkSize + 8;

  //Encoding
  if( strcmp(argv[1], "encode") == 0)
  {
    FILE* output = fopen(argv[4], "w+");
    if(output == NULL)
    {
      perror("Error opening output file\n");
      cleanUp();  
      exit(1);
    }

    //Copy header
    fseek(input, 0, SEEK_SET);
    char buffer[headerSize];
    fread(&buffer, headerSize, 1, input);
    fwrite(&buffer, headerSize, 1, output);

    // Read the secret message file
    FILE* message = fopen(argv[3], "r");
    if(message == NULL)
    {
      perror("No such file\n");
    }

    // Encode the secret into output file
    encode(input, output, message);
    printf("Encoding : SUCCESS\n");
    fclose(output);
    fclose(message);
  }

  //Decoding
  if( strcmp(argv[1], "decode") == 0)
  {
    FILE* output = fopen(argv[3], "w");
    if(output == NULL)
    {
      perror("Unable to create new file\n");
      cleanUp();
      exit(1);
    }
    // Decode the file
    decode(input, output);
    printf("Decoding : SUCCESS\n");
    fclose(output);
  }

  // Clean up
  cleanUp();  
  fclose(input);  
  return 0;
}

// This function verifies whether the file is a proper WAV file
// Input: 
//      wav : wav_subchunk (The structure is defined above)
void verifyWavheader(wav_subchunk wav)
{
  if(wav.RIFF[0] != 'R' ||
  wav.RIFF[1] != 'I' ||
  wav.RIFF[2] != 'F' ||
  wav.RIFF[3] != 'F') {
    printf("Not a RIFF file\n");
    cleanUp();
    exit(2);
  }

  if(wav.WAVE[0] != 'W' ||
  wav.WAVE[1] != 'A' ||
  wav.WAVE[2] != 'V' ||
  wav.WAVE[3] != 'E') {
    printf("Not a WAVE file\n");
    cleanUp();
    exit(2);
  }
  fileSize = wav.fileSize;
  printf("Wav header check : PASS\n");
}

// This function locates the 'fmt' and 'data' subchunks, and skips over others
// Input:
//       input: FILE*     (The input file/ballast)
//       chunk: subchunk  (Location to store the 'data' chunk meta information at)
// Return:
//       1 if 'data' chunk is found otherwise 0
int findSubchunk(subchunk chunk, FILE* input)
{
  //fmt subchunk
  if(fmtflag == 0 && (chunk.chunkLabel[0] == 'f' ||
  chunk.chunkLabel[1] == 'm' ||
  chunk.chunkLabel[2] == 't' ||
  chunk.chunkLabel[3] == ' ')) {
    //printf("fmt found\n");
    fmtflag = 1;
    fmt = (fmt_subchunk*)calloc(sizeof(fmt_subchunk), 1);
    fseek(input, -sizeof(subchunk), SEEK_CUR);
    fread(fmt, sizeof(fmt_subchunk), 1, input);
    return 0;
  }
  //data subchunk
  if(chunk.chunkLabel[0] == 'd' ||
  chunk.chunkLabel[1] == 'a' ||
  chunk.chunkLabel[2] == 't' ||
  chunk.chunkLabel[3] == 'a') {
    //printf("data found\n");
    data = (subchunk*)calloc(sizeof(subchunk), 1);
    data = memcpy(data, &chunk, sizeof(subchunk));
    return 1;
  }

  //Skip any other subchunk
  fseek(input, chunk.chunkSize, SEEK_CUR);
  return 0;
}


//Helper function to print Non-Null Terminated strings. Provide Stream and Number of characters to print.
void printBytes(char* buffer, size_t numBytes)
{
  size_t i = 0;
  for(i = 0; i < numBytes; i++)
    putchar(buffer[i]);
  printf("\n");
}

void cleanUp()
{
  free(fmt);
  free(data);
}

//Encoder
// Encodes the given 'message' into the 'input' file and produces 'output' file as a result
void encode(FILE* input, FILE* output, FILE* message)
{
  fseek(input, headerSize, SEEK_SET);
  fseek(output, headerSize, SEEK_SET);
  fseek(message, 0, SEEK_SET);
  //Determine padding limit, and number of bits to use
  struct stat fileStat;
  fstat(fileno(message), &fileStat);
  int32_t size = fileStat.st_size; 
  int32_t numRemaining = data->chunkSize/sizeof(int16_t);

  if(debug == 1){			
    printf("Samples available : %d\n", numRemaining);
    printf("Size of message : %d\n", size);
  }

  int samplesUsed = 8; 	//This means using 8 samples to store 1 byte of data (best case scenario)
  //So if samples used is 1, we are using 1 sample to store 1 byte of data(worst case scenario)

  // Determine how many bits we can safely use
  while(1)
  {
    if(samplesUsed == 1) 
      break;	
    if(numRemaining > size * samplesUsed)
      break;
    else 
      samplesUsed /= 2;
  }

  if(debug == 1)
    printf("Samples used : %d\n", samplesUsed);

  if(numRemaining < samplesUsed * size )
  {
    printf("Message too big, Insufficient padding available\n");
    cleanUp();
    fclose(input);
    fclose(output);
    fclose(message);
    exit(3);
  }

  //Store the size(int32_t) in first 32 samples
  size_t i = 0;
  for(i = 0; i < 4 ; i++)
  {
    int32_t tempSize = size;
    char* buffer = (char*)&tempSize;
    char current_byte = buffer[i];

    int j = 0;
    for(j = 0; j < 8; j++) //For every bit in a byte
    {
      char mask = 0x01;
      mask <<= j;
      char bit = mask & current_byte;
      bit >>= j;

      //This is for the case that 10000000 is right shifted
      bit &= 0x01;

      //Encode bit into image
      int16_t inputSample;
      fread(&inputSample, sizeof(int16_t), 1, input);
      //Make lsb 0
      inputSample &= 0xFFFE; 
      //Encode lsb
      inputSample |= bit;
      fwrite(&inputSample, sizeof(int16_t), 1, output) ;
      numRemaining--;
    }
  }


  int shiftVal = 8/samplesUsed;
  char globalMask = 0x01;

  //If 8 samples are used
  int16_t inputMask = 0xFFFE;
  if(samplesUsed == 4){
    globalMask = 0x02;
    inputMask = 0xFFFC;
  }
  if(samplesUsed == 2){
    globalMask = 0x0F;
    inputMask = 0xFFF0;
  }
  if(samplesUsed == 1){
    globalMask = 0xFF;
    inputMask = 0xFF00;
  }

  if(debug == 1)
    printf("Shift val : %d\n", shiftVal);

  //Encode message data into the next valid "size" samples
  for(i = 0; i < size; i++)
  {
    char current_byte;
    fread(&current_byte, sizeof(char), 1, message);
    int j = 0;
    for(j = 0; j < 8; j += shiftVal) //For every bit in a byte
    {
      char mask = globalMask;
      mask <<= j;
      char bit = mask & current_byte;
      bit >>= j;
      //If 10000000 is right shifted
      //bit &= 0x01;
      //Encode bit into image
      int16_t inputSample;
      fread(&inputSample, sizeof(int16_t), 1, input);

      //Make lsb 0
      inputSample &= inputMask; 
      //Encode lsb
      inputSample |= bit;

      fwrite(&inputSample, sizeof(int16_t), 1, output) ;
      numRemaining--;
    }
  }

  int16_t copy;
  for(i = 0; i < numRemaining ; i++)
  {
    fread(&copy, sizeof(int16_t), 1, input);
    fwrite(&copy, sizeof(int16_t), 1, output);
  }
}

//Decoder
void decode(FILE* input, FILE* output)
{
  fseek(input, headerSize, SEEK_SET);
  fseek(output, 0, SEEK_SET);
  int32_t numRemaining = data->chunkSize/sizeof(int16_t);

  //Extract the size from first 32 valid samples
  int32_t size = 0x00000000;
  size_t i = 0;
  for(i = 0; i < 32 ; i++)
  {
    //Read sample
    int16_t current_byte; 
    fread(&current_byte, sizeof(int16_t), 1, input) ;
    //Extract lsb
    int32_t bit = 0x0001 & current_byte;
    bit <<= i;
    size |= bit;
  }

  int samplesUsed = 8;

  // Determine how many samples were used from decoded size
  while(1)
  {
    if(samplesUsed == 1) 
      break;
    if(numRemaining > size * samplesUsed)
      break;
    else 
      samplesUsed /= 2;
  }

  if(numRemaining < samplesUsed * size )
  {
    printf("There was a problem decoding the file\n");
    cleanUp();
    exit(3);
  }

  if(debug == 1)
    printf("Samples used : %d\n", samplesUsed);

  int shiftVal = 8/samplesUsed;
  char mask = 0x01;
  //If 8 samples are used
  int16_t inputMask = 0xFFFE;
  if(samplesUsed == 4){
    mask = 0x02;
    inputMask = 0xFFFC;
  }
  if(samplesUsed == 2){
    mask = 0x0F;
    inputMask = 0xFFF0;
  }
  if(samplesUsed == 1){
    mask = 0x0001;
    inputMask = 0xFF00;
  }

  //Extract data from next "size" valid samples and write them onto output file
  for(i = 0; i < size ; i++)
  {
    char c = 0x00;
    int j = 0;
    for(j = 0; j < 8; j += shiftVal) //For every samplesUsed * bit(s) in a byte
    { 
      int16_t current_byte; 
      fread(&current_byte, sizeof(int16_t), 1, input) ;
      char bit = mask & current_byte;  
      bit <<= j;
      c |= bit;
    }
    fwrite(&c, sizeof(char), 1, output);
  }

}
