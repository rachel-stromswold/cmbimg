# CMBIMG
Load multiple images and combine them into one while performing basic manipulations
## SYNOPSIS
cmbimg [OPTIONS] [FILES] -o [OUTPUT FILE NAME]
## DESCRIPTION
cmbimg can load one or more images and apply the specified transformations before saving the output to the specified file.
## OPTIONS
### -h
Merge images horizontally (you can specify -v in the same command to merge both horizontally and vertically)

### -v
Merge images vertically (you can specify -h in the same command to merge both horizontally and vertically)

### -s x=[X_SCALE],y=[Y_SCALE] OR w=[WIDTH],h=[HEIGHT], --scale
Scale the next image by the factor of X_FACTOR in width and Y_FACTOR in height. If 'w' and 'h' are specified instead of 'x' and 'y' the scaling factor will be calculated to produce an image of the spcified size.

### -r x=[X],y=[Y],w=[WIDTH],h=[HEIGHT], --crop
Crop the next image so that it has a new width WIDTH and a new height HEIGHT. X and Y specify where cropping should start (from the top left corner of the image). If the specified WIDTH or HEIGHT goes off the screen then WIDTH and HEIGHT will be adjusted. An error may be thrown if cropping is impossible.

### -f , --force
Override warnings and errors such as "too many images". This may be unsafe, use with care.

### -c [COLS], --columns=COLS
Specify the number of columns to use when creating the output image.

### -n [NUM]
Specify the number of images to merge together. If specified, the first NUM images will be loaded and saved to the appropriate output image.

### -o [FNAME]
Specify the name of the file to write to. By default, the merged image is saved to "cmbimg_out.png".
## EXAMPLES
cmbimg foo.png bar.png -o out.png
cmbimg -n 2 foo.png bar.png out.png 
The above commands are equivalent and will save the horizontally merged images to the file out.png.

cmbimg --scale x=0.5,y=0.5 foo.png -o out.png
This commmand will scale the image foo.png to half its current width and height and save the result to out.png

cmbimg --scale x=0.5,y=0.5 foo.png --crop x=0,y=0,w=320,h=240 bar.png -o out.png
This commmand will scale the image foo.png to half its current width and height and then merge with the bar.png image cropped to a 320 by 240 image, the merged output will be saved to out.png

## SEE ALSO
[Lodepng](https://github.com/lvandeve/lodepng)
## BUGS
No known bugs.
## AUTHOR
Samuel Stromswold (samstromsw@gmail.com)
