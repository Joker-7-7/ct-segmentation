#include "Backend/ThirdParty/String.hpp"

#include <QUrl>

using namespace Viewer::Utils;

std::string String::WithoutUrlPrefixStd(const QString& path) noexcept
{
    return WithoutUrlPrefix(path).toStdString();
}

QString String::WithoutUrlPrefix(const QString& path) noexcept
{
    return path.startsWith("file:") ? QUrl(path).toLocalFile() : path;
}

QString String::ExtractFolderPath(const QString& path) noexcept
{
    std::string strippedPath = String::WithoutUrlPrefixStd(path);
    size_t latestSlash = strippedPath.find_last_of("/");
    std::string directory = strippedPath.substr(0, latestSlash + 1);
    return QString::fromStdString(directory);
}

QString String::WithoutExtension(const QString& fileName, std::string_view extension) noexcept
{
    qsizetype index = fileName.lastIndexOf(extension.data());
    return (index > 0 ? fileName.left(index) : fileName);
}
