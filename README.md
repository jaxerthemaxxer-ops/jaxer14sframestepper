# Macro Jump Counter (Geode mod)

Pick a `.gdr2` macro in Geometry Dash and see how many jumps it has. Made by jaxer14 for GD 2.2081 and Geode 5.11.2.

## Use
1. Press the **JC** button in the main menu.
2. Choose a `.gdr2` file.
3. Read the results. **Copy frames** copies the frame of every click.

## Build
Push this folder to a GitHub repository and run the **Build mod** workflow in the Actions tab.
Download **Build Output** and put the `.geode` file in `%LOCALAPPDATA%\GeometryDash\geode\mods`.

## Test the reader on its own
`g++ -O2 -std=c++20 -Isrc test/test_gdr2.cpp src/Gdr2.cpp -o test_gdr2 && ./test_gdr2 your_macro.gdr2`
