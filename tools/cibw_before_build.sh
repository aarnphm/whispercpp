set -xe

# Setup clang
if [[ $RUNNER_OS == "Linux" ]]; then
    yum install -y clang
elif [[ $RUNNER_OS == "macOS" ]]; then
    brew install llvm
fi
