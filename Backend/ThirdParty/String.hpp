#pragma once

#include <string>
#include <Qstring>

namespace Viewer::Utils
{
	/**
     * Structure for working with file path strings
     */
	struct String final
	{
        /**
         * Removes the URL prefix from the file path string.
         *
         * This method removes the "file:///" prefix from the file path string, if present,
         * and returns the remaining part of the path.
         *
         * @param  path The file path.
         * @return      The file path without the URL prefix.
         */
        static std::string WithoutUrlPrefixStd(const QString& path) noexcept;

        /**
         * Same as #WithoutUrlPrefixStd but returns a QString
         *
         * @param  path The file path.
         * @return      The file path without the URL prefix.
         */
        static QString WithoutUrlPrefix(const QString& path) noexcept;

        /**
         * Extracts the folder path from a file path.
         *
         * This method removes the URL prefix (if present) and extracts the folder path
         * from the file path string by identifying the last `/` character as the boundary.
         *
         * @param  path The file path
         * @return      The folder path
         */
        static QString ExtractFolderPath(const QString& path) noexcept;

        /**
         * Finds the latest occurence of #extension in #fileName and deleted it if it exists
         *
         * @param  fileName  Full name of file for which an extension should be removed
         * @param  extension File extension to be removed
         * @return           #fileName without the latest #extension occurence
         */
        static QString WithoutExtension(const QString& fileName, std::string_view extension) noexcept;
	};
}
