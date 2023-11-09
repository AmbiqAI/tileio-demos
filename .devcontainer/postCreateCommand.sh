# Install ARM GNU Toolchain

ARM_GCC_VERSION="13.2.Rel1"
ARM_GCC_URL="https://developer.arm.com/-/media/Files/downloads/gnu/$ARM_GCC_VERSION/binrel/arm-gnu-toolchain-$ARM_GCC_VERSION-x86_64-arm-none-eabi.tar.xz"
ARM_GCC_FOLDER="arm-gnu-toolchain"
ARM_GCC_SRC_PATH=/tmp/$ARM_GCC_FOLDER
ARM_GCC_DST_PATH=/usr/local/$ARM_GCC_FOLDER
curl -L $ARM_GCC_URL -o $ARM_GCC_SRC_PATH.tar.xz
mkdir -p $ARM_GCC_SRC_PATH
tar -xf $ARM_GCC_SRC_PATH.tar.xz -C $ARM_GCC_SRC_PATH --strip-components 1
sudo mv $ARM_GCC_SRC_PATH $ARM_GCC_DST_PATH
#export PATH=$PATH:"$ARM_GCC_DST_PATH/bin"
