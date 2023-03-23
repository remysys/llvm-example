
mkdir build
cd build
cmake ..
make
cd .. 

CLANG_INTERPRETER="./build/clang-interpreter"
LIBCODE="buildin.cpp"

TEST_DIR="./test"
test_file_list=$(ls $TEST_DIR)

total=$(echo "$test_file_list" | wc -w)

correct=0

echo "total test cases: $total"

for file in $test_file_list; do
    echo "testing $file"
    filename="$TEST_DIR/$file"
    ccode=$(cat $filename)

    # make $correct as the user input, you can change it if you like

    res=$(echo $correct | ($CLANG_INTERPRETER "$ccode" 2>&1 > /dev/null))
    gcc $filename $LIBCODE -o x.out
    expected=$(echo $correct | ./x.out)

    if [[ "$res" = "$expected" ]]; then
        echo "$file passed"
        correct=$(($correct + 1))
    fi
done
rm x.out
rm -fr build
echo "$correct/$total"