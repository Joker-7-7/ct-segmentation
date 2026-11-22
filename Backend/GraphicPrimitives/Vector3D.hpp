#pragma once

class vtkMatrix4x4;

namespace Dicom::Math
{
    /**
     * 3D vector including three coordinates: x, y, z
     * <p>
     * Used to work with geometry in 3D space
     * <p>
     *
     * Usage example:
     * @code
     * double coordinates[3] = { 1.25, 2.33, 3.94 };
     * Vector3D vector(coordinates);
     * vector.round();
     * @endcode
     */
    struct Vector3D final
    {
        /**
         * X coordinate of a vector in space
         */
        double x = 0;

        /**
         * Y coordinate of a vector in space
         */
        double y = 0;

        /**
         * Z coordinate of a vector in space
         */
        double z = 0;

        /**
         * Ctor
         */
        Vector3D() = default;

        /**
         * Ctor
         *
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         */
        Vector3D(double x, double y, double z) noexcept;

        /**
         * Ctor
         *
         * @param value pointer to a 3-element array (x, y, z)
         */
        explicit Vector3D(const double* value) noexcept;

        /**
         * Finds the angle between two vectors (in degrees)
         *
         * @param  lhs First vector
         * @param  rhs Second vector
         * @return     The angle between #lhs and #rhs in degrees
         */
        [[nodiscard]] static double angleBetween(const Vector3D& lhs, const Vector3D& rhs) noexcept;

        /**
         * Reinterprets the point as an array
         *
         * @return pointer to the 'x' coordinate
         */
        double* ptr() noexcept;

        /**
         * Reinterprets the point as a const array
         *
         * @return const pointer to the 'x' coordinate
         */
        const double* ptr() const noexcept;

        /**
         * Assignment operator.
         *
         * @param  value pointer to a 3-element array with [x, y, z] coordinates.
         * @return       reference to the current instance
         *
         * @attention    passing pointer to invalid or forbidden memory cause undefined behaviour
         */
        Vector3D& operator=(const double* value) noexcept;

        /**
         * Rounds all coordinates to the nearest integer values
         */
        void round() noexcept;

        /**
         * Normalizes the current vector.
         */
        void normalize() noexcept;

        /**
         * Gets normalized vector from the current instance.
         *
         * @return <code>(0, 0, 0)</code> if the current instance is <code>(0, 0, 0)</code>,
         *         <code>(x / len, y / len, z / len)</code> otherwise, where the <code>len</code>
         *         is the current vector length.
         */
        Vector3D normalized() const noexcept;

        /**
         * Calculates a length (Euclidean norm) of a vector.
         * @return Length of a vector <code>sqrt(x*x + y*y + z*z)</code>
         */
        double length() const noexcept;

        /**
         * Applies the passed matrix to the current vector in a specific way.
         * Lets assume the matrix has the following structure:
         *
         * |a11 a12 a13 a14|
         * |a21 a22 a23 a24|
         * |a31 a32 a33 a34|
         * |a41 a42 a43 a44|
         *
         * Then this function will return the result of multiplication
         * of the top left matrix 3x3 minor by the current vector.
         *
         * So the result will be:
         * (a11 * x + a12 * y + a13 * z, a21 * x + a22 * y + a22 * z, a31 * x + a32 * y + a33 * z)
         *
         * @param  matrix The matrix to be applied
         * @return        The product of the top left matrix 3x3 minor and the current vector
         */
        // Vector3D applyMatrix(vtkMatrix4x4* matrix) const noexcept;
    };

    /**
     * Checks either the two vectors are equal
     *
     * @param  lhs Vector to be compared
     * @param  rhs Vector to be compared with
     * @return     <code>true</code> if the vector is equal to the current instance
     *             <code>false</code> otherwise
     */
    bool operator==(const Vector3D& lhs, const Vector3D& other) noexcept;

    /**
     * Subtraction operator
     *
     * @param  lhs Vector to be substracted from
     * @param  rhs Vector to be substracted
     * @return     subtraction result
     */
    Vector3D operator-(const Vector3D& lhs, const Vector3D& rhs) noexcept;

    /**
     * Addition operator
     *
     * @param  lhs Vector to be added to
     * @param  rhs Vector to be added
     * @return     addition result
     */
    Vector3D operator+(const Vector3D& lhs, const Vector3D& rhs) noexcept;

    /**
     * Multiplies the current vector by provided factor.
     *
     * @param   vec    Vector to be multiplied
     * @param   factor Factor to multiple by
     * @returns        <code>(vec.x * factor, vec.y * factor, vec.z * factor)</code> vector
     */
    Vector3D operator*(const Vector3D& vec, double factor) noexcept;

    /**
     * Multiplies the current vector by provided factor.
     *
     * @param   factor Factor to multiple by
     * @param   vec    Vector to be multiplied
     * @returns        <code>(vec.x * factor, vec.y * factor, vec.z * factor)</code> vector
     */
    Vector3D operator*(double factor, const Vector3D& vec) noexcept;

    /**
     * Divides the current vector by provided factor.
     *
     * @param   vec    Vector to be divided
     * @param   factor Factor to divide by
     * @returns        <code>(vec.x / factor, vec.y / factor, vec.z / factor)</code> vector
     */
    Vector3D operator/(const Vector3D& vec, double factor) noexcept;

    /**
     * Negates the vector
     *
     * @param  vec Vector to be negated
     * @return     <code>(-vec.x, -vec.y, -vec.z)</code> vector
     */
    Vector3D operator-(const Vector3D& vec) noexcept;
}
