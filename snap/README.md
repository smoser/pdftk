[PDFtk](https://www.pdflabs.com/tools/pdftk-server/) is a useful program
for manipulating/creating pdf.  It is not available in 18.04.

Creation of this followed [Building Snaps From Archived Packages](https://larry-price.com/blog/2016/12/07/creating-a-snap-from-existing-deb-packages/).

If you use this snap and are happy with it, please upvote
the snap answer to the [askubuntu question](https://askubuntu.com/questions/1028522/how-can-i-install-pdftk-in-ubuntu-18-04-bionic).

## Build ##
Note that the build of this snap does not currently use the source code
here.  Instead, it installs pdftk and its dependecies in Ubuntu 16.04
and makes a snap from those.

To build:

    snapcraft

## Installation ##

    snap install pdftk

Or, from the snapcraft built source:

    snap install --devmode pdftk_*.snap

## Revision control / Git ##
To my knowledge there is no available revision control for pdftk.
Thus, I have used 'git-ubuntu' to import Ubuntu and Debian's history.
I did that on version `0.7.4+git82.3512465` with:

    git ubuntu import -v --directory=pdftk --lp-user=smoser --no-push pdftk
