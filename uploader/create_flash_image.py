# Usage: python create_flash_image.py file.mem

def format_c_array(data, var_name="flash_image"):
    lines = []
    lines.append(f"const uint8_t {var_name}[] = {{")
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_bytes = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f"  {hex_bytes},")
    lines.append("};")
    lines.append(f"const size_t {var_name}_size = sizeof({var_name});")
    return '\n'.join(lines)

if __name__ == "__main__":
    with open("ISD2360_Example_Project_2.mem", "rb") as f:
        data = f.read()
    with open("out.c", "w") as out:
        out.write(format_c_array(data))