/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec: 113475 Name: Carolina Silva
// NMec: 114624 Name: Sebastião Teixeira
// 
// 
// 
// Date: 2023-11-03
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
// 
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() { ///
  return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success = 
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
// 
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
// 
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
// 
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Add more instrumentation names here...
  InstrName[1] = "imagelocatesubimage";
  InstrName[2] = "imageblur";
 


  
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...
#define IMAGELOCATESUBIMAGE InstrCount[1]
#define IMAGEBLUR InstrCount[2]


// TIP: Search for PIXMEM or InstrCount to see where it is incremented!


static inline uint8 roundPixel(double pixel) {
  return (uint8) (pixel + 0.5);
}


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) { ///
  assert (width >= 0);
  assert (height >= 0);
  assert (0 < maxval && maxval <= PixMax);
  Image img = (Image)malloc(sizeof(struct image));
  if (img == NULL) {
    errCause = "Memory allocation error";
    return NULL;
  }

  img->width = width;
  img->height = height;
  img->maxval = maxval;

  // Allocate memory for the pixel data
  img->pixel = (uint8*)malloc(sizeof(uint8) * width * height);
  if (img->pixel == NULL) {
    free(img); // Clean up the partially allocated image structure
    errCause = "Memory allocation error for pixel data";
    return NULL;
  }

  // Initialize the pixel data, set all pixels to 0 (black)
  for (int i = 0; i < width * height; i++) {
    img->pixel[i] = 0;
  }

  return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image* imgp) { ///
  assert (imgp != NULL);
  // Insert your code here!

  if (*imgp != NULL) {
    // Deallocate the pixel data
    free((*imgp)->pixel);
    // Deallocate the image structure itself
    free(*imgp);
    *imgp = NULL;
  }
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) { ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success = 
  check( (f = fopen(filename, "rb")) != NULL, "Open failed" ) &&
  // Parse PGM header
  check( fscanf(f, "P%c ", &c) == 1 && c == '5' , "Invalid file format" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &w) == 1 && w >= 0 , "Invalid width" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d ", &h) == 1 && h >= 0 , "Invalid height" ) &&
  skipComments(f) >= 0 &&
  check( fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax , "Invalid maxval" ) &&
  check( fscanf(f, "%c", &c) == 1 && isspace(c) , "Whitespace expected" ) &&
  // Allocate image
  (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
  // Read pixels
  check( fread(img->pixel, sizeof(uint8), w*h, f) == w*h , "Reading pixels" );
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) { ///
  assert (img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success =
  check( (f = fopen(filename, "wb")) != NULL, "Open failed" ) &&
  check( fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed" ) &&
  check( fwrite(img->pixel, sizeof(uint8), w*h, f) == w*h, "Writing pixels failed" ); 
  PIXMEM += (unsigned long)(w*h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
  assert (img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
  assert (img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
  assert (img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) { ///
  assert (img != NULL);
  assert (min != NULL);
  assert (max != NULL);
  
  *min = PixMax; // Configurando o mínimo para o valor máximo possível inicialmente
    *max = 0;      // Configurando o máximo para 0 inicialmente

    for (int i = 0; i < img->width * img->height; i++) {
        if (img->pixel[i] < *min) {
            *min = img->pixel[i]; // Atualiza o mínimo se encontrar um valor menor
        }
        if (img->pixel[i] > *max) {
            *max = img->pixel[i]; // Atualiza o máximo se encontrar um valor maior
        }
    }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
  assert (img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  // Verifica se as coordenadas superior esquerda e inferior direita do retângulo estão dentro dos limites da imagem
    return ImageValidPos(img, x, y) && ImageValidPos(img, x + w - 1, y + h - 1);
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to 
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel. 
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
  int index;
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  index = y*img->width + x;
  if (index < 0) index = 0;
  if (index >= img->width*img->height) index = img->width*img->height - 1;

  assert (0 <= index && index < img->width*img->height);
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  return img->pixel[G(img, x, y)];
} 

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
  assert (img != NULL);
  assert (ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
} 


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) { ///
  assert (img != NULL);
  // Insert your code here!
	for (int x = 0; x < img->width; x++)
		for (int y = 0; y < img->height; y++)
			ImageSetPixel(img, x, y, img->maxval - ImageGetPixel(img, x, y));
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
  assert (img != NULL);
  // Insert your code here!
	for (int x = 0; x < img->width; x++)
		for (int y = 0; y < img->height; y++)
			ImageSetPixel(img, x, y, ImageGetPixel(img, x, y) >= thr ? img->maxval : 0);
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) { ///
  assert (img != NULL);
  // ? assert (factor >= 0.0);
  assert(factor >= 0.0); // Se desejar garantir que o fator seja positivo
    
    int width = img->width;
    int height = img->height;

	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++) {
        	// Saturar o valor para o máximo, se necessário
			ImageSetPixel(img, x, y, roundPixel(ImageGetPixel(img, x, y) * factor));
    		if (ImageGetPixel(img, x, y) > img->maxval)
				ImageSetPixel(img, x, y, img->maxval);
	}
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
/// 
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint: 
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees clockwise.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) { ///
  assert (img != NULL);
  int width = img->width;
  int height = img->height;
  uint8 maxval = img->maxval;

  // Criar uma nova imagem com dimensões trocadas para a rotação
  Image rotatedImage = ImageCreate(height, width, maxval);
  if (rotatedImage == NULL) {
    errCause = "Memory allocation error for rotated image";
	return NULL;
  }
  for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Obter o valor do pixel da imagem original
            uint8 currentPixel = ImageGetPixel(img, j, i);
            // Preencher a nova imagem com os valores rotacionados
            ImageSetPixel(rotatedImage, i, width - j - 1, currentPixel);
        }
    }

    return rotatedImage;

}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) { ///
  assert (img != NULL);
  int width = img->width;
    int height = img->height;
    uint8 maxval = img->maxval;

    // Cria uma nova imagem
    Image mirroredImage = ImageCreate(width, height, maxval);
    if (mirroredImage == NULL) {
        errCause = "Memory allocation error for mirrored image";
        return NULL;
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Obtém o valor do pixel da imagem original
            uint8 currentPixel = ImageGetPixel(img, j, i);
            // Preenche a nova imagem com os valores refletidos horizontalmente
            ImageSetPixel(mirroredImage, width - j - 1, i, currentPixel);
        }
    }

    return mirroredImage;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
/// 
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) { ///
  assert (img != NULL);
  assert (ImageValidRect(img, x, y, w, h));
  uint8 maxval = img->maxval;
    // Cria uma nova imagem com as dimensões do recorte
    Image croppedImage = ImageCreate(w, h, maxval);
    if (croppedImage == NULL) {
        errCause = "Memory allocation error for cropped image";
        return NULL;
    }

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            // Obtém o valor do pixel da imagem original na área recortada
            uint8 currentPixel = ImageGetPixel(img, x + j, y + i);
            // Preenche a nova imagem com os valores da área recortada
            ImageSetPixel(croppedImage, j, i, currentPixel);
        }
    }

    return croppedImage;
}


/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));
  int width2 = img2->width;
    int height2 = img2->height;
	img1->maxval = img1->maxval > img2->maxval ? img1->maxval : img2->maxval; // O valor máximo da imagem maior será o maior entre os valores máximos das duas imagens
    for (int i = 0; i < height2; i++) {
        for (int j = 0; j < width2; j++) {
            // Obtém o valor do pixel da imagem a ser colada
            uint8 currentPixel = ImageGetPixel(img2, j, i);
            // Preenche a imagem maior na posição correta com os valores da imagem a ser colada
            ImageSetPixel(img1, x + j, y + i, currentPixel);
        }
    }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidRect(img1, x, y, img2->width, img2->height));

  int width2 = img2->width;
    int height2 = img2->height;
	img1->maxval = img1->maxval > img2->maxval ? img1->maxval : img2->maxval; // O valor máximo da imagem maior será o maior entre os valores máximos das duas imagens

    for (int i = 0; i < height2; i++) {
        for (int j = 0; j < width2; j++) {
            // Obtém o valor do pixel das duas imagens
            uint8 pixel1 = ImageGetPixel(img1, x + j, y + i);
            uint8 pixel2 = ImageGetPixel(img2, j, i);

            // Calcula o novo valor do pixel após a mesclagem
            double _blendedPixel = alpha * pixel2 + (1.0 - alpha) * pixel1;
			uint8 blendedPixel = roundPixel(_blendedPixel); // Arredondamento (saturação

            // Satura o valor resultante para o intervalo [0, maxval]
            uint8 finalPixel = (blendedPixel > img1->maxval) ? img1->maxval : (uint8)blendedPixel;

            // Atualiza o pixel na imagem maior (img1)
            ImageSetPixel(img1, x + j, y + i, finalPixel);
        }
    }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);
  assert (ImageValidPos(img1, x, y));
  
  int width2 = img2->width;
    int height2 = img2->height;

    // Verifica se a subimagem cabe dentro da imagem maior
    if (!ImageValidRect(img1, x, y, width2, height2)) {
        return 0;
    }

    for (int i = 0; i < height2; i++) {
        for (int j = 0; j < width2; j++) {
            // Obtém o valor do pixel das duas imagens
            uint8 pixel1 = ImageGetPixel(img1, x + j, y + i);
            uint8 pixel2 = ImageGetPixel(img2, j, i);
            //Contador imagelocatesubimage
            IMAGELOCATESUBIMAGE += 1;
            // Se um único pixel não corresponder, retorna 0 (falso)
            if (pixel1 != pixel2) {
                return 0;
            }
        }
    }

    // Se todos os pixels corresponderem, retorna 1 (verdadeiro)
    return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) { ///
  assert (img1 != NULL);
  assert (img2 != NULL);

  int width1 = img1->width;
  int height1 = img1->height;
  int width2 = img2->width;
  int height2 = img2->height;

  // Verifica se as dimensões da subimagem são menores ou iguais às dimensões da imagem maior
    if (width2 > width1 || height2 > height1) {
        return 0;
    }

    for (int i = 0; i < height1 - height2; i++) {
        for (int j = 0; j < width1 - width2; j++) {
            // Verifica se há correspondência da subimagem na posição atual
            if (ImageMatchSubImage(img1, j, i, img2)) {
                *px = j; // Define a posição x onde a subimagem foi localizada
                *py = i; // Define a posição y onde a subimagem foi localizada

                return 1; // Retorna 1 (verdadeiro) indicando uma correspondência
            }
        }
    }

    // Se nenhuma correspondência for encontrada, retorna 0 (falso)
    return 0;
}



/// Filtering

/// Blur an image by applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void _ImageBlur_1(Image img, int dx, int dy) { ///
  // Insert your code here!
  	int width = img->width;
    int height = img->height;
	int totalPixels = width * height;
	

    Image tempImg = ImageCreate(width, height, img->maxval); // Criar uma imagem temporária

	int hSum = 0;
	// For first pixel
	for (int px = 0; px <= dx /* as px < dx+1 */; px++) {
		for (int py = 0; py <= dy; py++) {
			hSum += ImageGetPixel(img, px, py);
			IMAGEBLUR++;
		}
	}

	int previous_y1 = 0;
	int previous_y2 = dy;
	int y1 = 0;
	int y2 = dy;
    for (int y = 0; y < height; ) {
		ImageSetPixel(tempImg, 0, y, roundPixel((double) hSum / ((dx + 1) * (y2 - y1 + 1))));

		// For remaining pixels in each row
		int wSum = hSum;
		int previous_x1 = 0;
		int previous_x2 = dx;
		for (int x = 1; x < width; x++) {
			int x1 = x - dx > 0 ? x - dx : 0;
			int x2 = x + dx < height ? x + dx : width - 1;

			
			// Subtract the leftmost pixel from the sum
			// except if the leftmost pixel is the same
			if (x1 != previous_x1) {
				for (int py = y1; py <= y2; py++) {
					wSum -= ImageGetPixel(img, previous_x1, py);
					IMAGEBLUR++;
				}
			}
			if (x2 != previous_x2) {
				for (int py = y1; py <= y2; py++) {
					wSum += ImageGetPixel(img, x2, py);
					IMAGEBLUR++;
				}
			}
			
			previous_x1 = x1;
			previous_x2 = x2;
			ImageSetPixel(tempImg, x, y, roundPixel((double) wSum / ((x2 - x1 + 1) * (y2 - y1 + 1))));
		}
		
		y++; // Next row
		previous_y1 = y1;
		previous_y2 = y2;
		y1 = y - dy > 0 ? y - dy : 0;
		y2 = y + dy < height ? y + dy : height - 1;

		// For first pixel in next row
		if (y1 != previous_y1) {
			for (int px = 0; px <= dx; px++) {
				hSum -= ImageGetPixel(img, px, previous_y1);
				IMAGEBLUR++;
			}
		}

		if (y2 != previous_y2) {
			for (int px = 0; px <= dx; px++) {
				hSum += ImageGetPixel(img, px, y2);
				IMAGEBLUR++;
			}
		}
		

    }

    // Copiar a imagem temporária de volta para a imagem original
    for (int i = 0; i < totalPixels; i++) {
        img->pixel[i] = tempImg->pixel[i];
    }

    // Destruir a imagem temporária
    ImageDestroy(&tempImg);
}

void _ImageBlur_2(Image img, int dx, int dy) {
	long *summed_table = (long *) malloc(sizeof(long) * img->width * img->height);
	
	uint8 *firstPixel = img->pixel;
	uint8 *last_currentRow_Pixel = firstPixel + img->width;
	uint8 *lastPixel = firstPixel + img->width * img->height;
	uint8 *currentPixel = firstPixel;
	
	long *firstSum = summed_table;
	long *currentSum = firstSum;
	
	// For first row
	// For first pixel of first row
	*currentSum = *currentPixel;
	currentPixel++;
	currentSum++;
	IMAGEBLUR++;
	PIXMEM++;
	// For remaining pixels of first row
	while(currentPixel < last_currentRow_Pixel) {
		*currentSum = *currentPixel + *(currentSum - 1);
		currentPixel++;
		currentSum++;
		IMAGEBLUR += 2;
		PIXMEM++;
	}
	last_currentRow_Pixel += img->width;

	// For remaining rows
	while(currentPixel < lastPixel) {
		// For first pixel of remaining rows
		*currentSum = *currentPixel + *(currentSum - img->width);
		currentPixel++;
		currentSum++;
		IMAGEBLUR += 2;
		PIXMEM++;

		// For remaining pixels of remaining rows
		while (currentPixel < last_currentRow_Pixel) {
			*currentSum = *currentPixel + *(currentSum - img->width) + *(currentSum - 1) - *(currentSum - img->width - 1);
			currentPixel++;
			currentSum++;
			IMAGEBLUR += 4;
			PIXMEM++;
		}
		last_currentRow_Pixel += img->width;
	}


	
	// Finaly, summed table is ready to use
	for(int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			int x1 = x - dx - 1;
			int x2 = x + dx < img->width ? x + dx : img->width - 1;
			int y1 = y - dy - 1;
			int y2 = y + dy < img->height ? y + dy : img->height - 1;

			int area = (1+x2-(x1>=0 ? x1 + 1 : 0)) * (1+y2-(y1>=0 ? y1 + 1 : 0));

			IMAGEBLUR++;
			if (x1 >= 0) IMAGEBLUR++;
			if (y1 >= 0) IMAGEBLUR++;
			if (x1 >= 0 && y1 >= 0) IMAGEBLUR++;
			ImageSetPixel(img, x, y,
					roundPixel((double) (
							*(summed_table + G(img, x2, y2))
							- (x1 >= 0 ? *(summed_table + G(img, x1, y2)) : 0)
							- (y1 >= 0 ? *(summed_table + G(img, x2, y1)) : 0)
							+ (x1 >= 0 && y1 >= 0 ? *(summed_table + G(img, x1, y1)) : 0) 
							)
						/ area)
					);
		}
	}

}

void ImageBlur(Image img, int dx, int dy) { ///
  assert (img != NULL);
  assert (dx >= 0);
  assert (dy >= 0);
  // Insert your code here!
  _ImageBlur_2(img, dx, dy);
}

