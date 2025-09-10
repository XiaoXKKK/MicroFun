# 定义压缩包名称，可自定义
ZIP_NAME="1.zip"

# 获取所有被Git跟踪的文件列表，并排除.git目录
# 然后将这些文件压缩到指定的zip包中
git ls-files | grep -v '^.git/' | zip "$ZIP_NAME" -@

echo "已将所有Git跟踪的文件打包到: $ZIP_NAME"