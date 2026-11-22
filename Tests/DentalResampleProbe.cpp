#include "../Inference/Volume.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
        {
            return 2;
        }
        std::ifstream input(argv[1], std::ios::binary);
        Dental::Volume v;
        Dental::Shape target;
        int order, axis;
        input.read(reinterpret_cast<char*>(v.shape.data()), 12);
        input.read(reinterpret_cast<char*>(target.data()), 12);
        input.read(reinterpret_cast<char*>(&order), 4);
        input.read(reinterpret_cast<char*>(&axis), 4);
        v.data.resize(Dental::Count(v.shape));
        input.read(reinterpret_cast<char*>(v.data.data()), v.data.size() * sizeof(float));
        if (input.fail())
        {
            throw std::runtime_error("Invalid fixture");
        }
        const auto result = Dental::Resize(v, target, order, axis);
        std::ofstream output(argv[2], std::ios::binary);
        output.write(
            reinterpret_cast<const char*>(result.data.data()), result.data.size() * sizeof(float)
        );
        return output ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
