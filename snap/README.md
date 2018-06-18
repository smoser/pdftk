[PDFtk](https://www.pdflabs.com/tools/pdftk-server/) is a useful program
for manipulating/creating pdf.  It is not available in 18.04.

Creation of this followed [Building Snaps From Archived Packages](https://larry-price.com/blog/2016/12/07/creating-a-snap-from-existing-deb-packages/).

## Build ##
Note that the build of this does not currently actually use the local source
but rather just packs up what is in the current build system.

To build:

    snapcraft

## Installation ##

    snap install --devmode pdftk_*.snap

## Revision control / Git ##
To my knowledge there is no available revision control for pdftk.
Thus, I have used 'git-ubuntu' to import Ubuntu and Debian's history.
I did that on version `0.7.4+git82.3512465` with:

    git ubuntu import -v --directory=pdftk --lp-user=smoser --no-push pdftk
