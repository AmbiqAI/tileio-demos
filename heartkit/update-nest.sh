#!/bin/bash

####################################################################################
# Completely rebuilds the neuralspot library and updates the project with the new
# files. This script should be run from the root of the project.
####################################################################################

NS_COMMIT="dbeb9f7" # working: "dbeb9f7", latest: "e40475c5"
AS_VERSION="R4.4.1" # working: "R4.4.1", latest: "R4.5.0"
PLATFORM="apollo4p_blue_kxr_evb"
COMPILER="arm-none-eabi"
NESTEGG="rpc_client"
MAIN_APP="main"

# Get current directory of script this will be the root of the project
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJ_DIR=$DIR
NS_DIR=$PROJ_DIR/../neuralspot
NEST_DIR=$NS_DIR/nest
AUTOGEN_MK_FILE="autogen_${PLATFORM}_${COMPILER}.mk"

echo "Cleaning project"
rm $PROJ_DIR/AUTOGEN_MK_FILE
rm -rf $PROJ_DIR/build/
rm -rf $PROJ_DIR/includes/
rm -rf $PROJ_DIR/libs/
rm -rf $PROJ_DIR/make/
rm -rf $PROJ_DIR/pack/
rm -rf $PROJ_DIR/src/ns-core/

echo "Checking out neuralspot ($NS_COMMIT)"
cd $NS_DIR
git checkout $NS_COMMIT

echo "Building neuralspot"
make clean
make PLATFORM=$PLATFORM \
    NESTEGG=$NESTEGG \
    NESTDIR=$NEST_DIR \
    AS_VERSION=$AS_VERSION \
    nestall
# Check if the build was successful
if [ $? -ne 0 ]; then
    echo "neuralspot build failed"
    exit 1
fi

echo "Copying files to project"
cp -r $NEST_DIR/includes/ $PROJ_DIR/includes
cp -r $NEST_DIR/libs/ $PROJ_DIR/libs
cp -r $NEST_DIR/make/ $PROJ_DIR/make
cp -r $NEST_DIR/pack/ $PROJ_DIR/pack
cp -r $NEST_DIR/src/ns-core/ $PROJ_DIR/src/ns-core
cp $NEST_DIR/$AUTOGEN_MK_FILE $PROJ_DIR/$AUTOGEN_MK_FILE
# Replace the NESTEGG with the MAIN_APP in the autogen file
sed -i '' "s/$NESTEGG/$MAIN_APP/g" $PROJ_DIR/$AUTOGEN_MK_FILE

echo "Setting local overrides"
echo "AS_VERSION := $AS_VERSION" > $PROJ_DIR/make/local_overrides.mk
echo "PLATFORM := $PLATFORM" >> $PROJ_DIR/make/local_overrides.mk
echo "NS_VERSION := $NS_COMMIT" >> $PROJ_DIR/make/local_overrides.mk

echo "Building project"
cd $PROJ_DIR
make -j
