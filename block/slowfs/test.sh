#!/usr/bin/env bash

TMP_EXPECTED_FILE="/tmp/slowfs_test_tmp_expected"
TMP_FILE="/tmp/slowfs_test_tmp_output"

fail() {
    local i=0
    echo
    echo "Stack trace:"
    while frame_info=$(caller $i); do
        local line_number=$(echo "$frame_info" | awk '{print $1}')
        local function_name=$(echo "$frame_info" | awk '{print $2}')
        local file_name=$(echo "$frame_info" | awk '{print $3}')
        echo "  at $function_name() in $file_name:$line_number"
        ((i++))
    done
    echo
    exit 1
}

assert_ret() {
    local expected="$1"
    local cmd="$2"

    echo "$expected" >"$TMP_EXPECTED_FILE"
    sudo sh -c "$cmd" >"$TMP_FILE"

    diff -u --color=always "$TMP_EXPECTED_FILE" "$TMP_FILE"
    if [ "$?" -ne 0 ]; then
        fail
    fi
}

assert_code() {
    local expected="$1"
    local cmd="$2"

    echo "$expected" >"$TMP_EXPECTED_FILE"
    sudo sh -c "$cmd" >/dev/null
    echo "$?" >"$TMP_FILE"

    diff -u --color=always "$TMP_EXPECTED_FILE" "$TMP_FILE"
    if [ "$?" -ne 0 ]; then
        fail
    fi
}

if [ "$EUID" -eq 0 ]; then
    echo "Don't run this script as root"
    exit
fi

# Cleanup
mkdir -p ./mnt
sudo umount ./mnt 2>/dev/null
sudo rmmod slowfs 2>/dev/null

###############
# Start Tests #
###############

sudo dmesg -C
sudo insmod slowfs.ko
# sudo mount -t slowfs nodev ./mnt
assert_ret 1 "sudo dmesg | grep 'slowfs: init' | wc -l"


# # Cleanup
# sudo umount ./mnt
sudo rmmod slowfs
assert_ret 1 "sudo dmesg | grep 'slowfs: exit' | wc -l"

# End Tests
echo 'PASS'
