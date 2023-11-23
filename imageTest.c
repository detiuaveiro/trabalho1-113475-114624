// imageTest - A program that performs some image processing.
//
// This program is an example use of the image8bit module,
// a programming project for the course AED, DETI / UA.PT
//
// You may freely use and modify this code, NO WARRANTY, blah blah,
// as long as you give proper credit to the original and subsequent authors.
//
// João Manuel Rodrigues <jmr@ua.pt>
// 2023

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image8bit.h"
#include "instrumentation.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    error(1, 0, "Usage: imageTest input.pgm output.pgm");
  }

  ImageInit();
  
  printf("# LOAD image\n");
  InstrReset(); // to reset instrumentation
  Image img1 = ImageLoad(argv[1]);
  if (img1 == NULL) {
    error(2, errno, "Loading %s: %s", argv[1], ImageErrMsg());
  }
  InstrPrint(); // to print instrumentation

  // Try changing the behaviour of the program by commenting/uncommenting
  // the appropriate lines.

  //img2 = ImageCrop(img1, ImageWidth(img1)/4, ImageHeight(img1)/4, ImageWidth(img1)/2, ImageHeight(img1)/2);
  printf("# ROTATE image\n");
  InstrReset();
  Image img2 = ImageRotate(img1);
  if (img2 == NULL) {
    error(2, errno, "Rotating img2: %s", ImageErrMsg());
  }
  InstrPrint();

  //ImageNegative(img2);
  //ImageThreshold(img2, 100);
  printf("# BRIGHTEN image\n");
  InstrReset();
  ImageBrighten(img2, 1.3);

  if (ImageSave(img2, argv[2]) == 0) {
    error(2, errno, "%s: %s", argv[2], ImageErrMsg());
  }
  InstrPrint();

  

  // Testes para a função ImageLocateSubImage
  /*
  Image img3 = ImageLoad("./test/original.pgm");
  Image img4 = ImageLoad("./test/stripes8.pgm");
  
  // Testes para a função ImageBlur
  Image img5 = ImageLoad("./test/chess8.pgm");

  
  
  printf("# LocateSubImage image\n");
  InstrReset(); // para resetar a instrumentação
  int x, y;
  int success = ImageLocateSubImage(img3, &x, &y, img4);
  if (!success) {
    error(2, errno, "Locating sub-image: %s", ImageErrMsg());
  }
  InstrPrint(); // para imprimir a instrumentação

  
  // image blur
  printf("# Blur image\n");
  InstrReset(); // para resetar a instrumentação
  ImageBlur(img5, 1, 1);
  InstrPrint(); // para imprimir a instrumentação
  */
  ImageDestroy(&img1);
  ImageDestroy(&img2);
  /*
  ImageDestroy(&img3);
  ImageDestroy(&img4);
  ImageDestroy(&img5);
  */

  return 0;
}

