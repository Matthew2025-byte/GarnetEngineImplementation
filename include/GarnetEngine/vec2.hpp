#pragma once
#include <string>
namespace Garnet {
    struct vec2 {
        float x, y;
        vec2() : x(0), y(0) {};
        vec2(float x, float y) : x(x), y(y) {};


        /**
         * @return The magnitude (length) of the vector
         */
        float magnitude() const;


        /**
         * @return The normalized vector (unit vector)
         */
        vec2 normalized() const;


        /**
         * @param other The other vector to calculate the dot product with
         * @return The dot product of this vector and the other vector
         */
        float dot(const vec2& other) const;

        /**
         * @return A string representation of the vector
         */
        std::string str() const;


        /**
         * @param other The other vector to add
         * @return The result of vector addition
         */
        vec2 operator + (const vec2& other) const;


        /**
         * @param other The other vector to subtract
         * @return The result of vector subtraction
         */
        vec2 operator - (const vec2& other) const;

        /**
         * @param scalar The scalar to multiply by
         * @return The result of scalar multiplication
         */
        vec2 operator * (float scalar) const;

        /**
         * @param scalar The scalar to divide by
         * @return The result of scalar division
         */
        vec2 operator / (float scalar) const;
    };
}