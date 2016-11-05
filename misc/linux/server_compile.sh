#!/bin/bash
localPATH=`pwd`
export BUILD_CLIENT="0"
export BUILD_SERVER="1"
export USE_CURL="1"
export USE_CODEC_OPUS="1"
export USE_VOIP="1"
export COPYDIR="~/spearmint"
ENGINEREMOTE="https://github.com/zturtleman/spearmint.git"
ENGINELOCAL="/tmp/spearmintcompile"
JOPTS="-j2" 
echo "This process requires you to have the following installed through your distribution:
 make
 git
 and all of the dependencies necessary for the Spearmint server.
 If you do not have the necessary dependencies this script will bail out.
 Please post a message to https://forum.clover.moe/ asking for help and include whatever error messages you received during the compile phase.
 Please edit this script. Inside you will find a COPYDIR variable you can alter to change where Spearmint will be installed to."
while true; do
        read -p "Are you ready to compile Spearmint in the $ENGINELOCAL directory, and have it installed into $COPYDIR? " yn
case $yn in
        [Yy]* )
if  [ -x "$(command -v git)" ] && [ -x "$(command -v make)" ] ; then
        git clone $ENGINEREMOTE $ENGINELOCAL && cd $ENGINELOCAL && make $JOPTS && make copyfiles && cd $localPATH && rm -rf $ENGINELOCAL
fi
        exit;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
esac
done
