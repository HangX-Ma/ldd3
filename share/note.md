
# Note

## VSCode Host

If you use VSCode and have installed the `C/C++` IntelliSense plugin, you can Refer to the following configuration.

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                /* ----------- HOST DEFINITION ----------- */
                // "/usr/src/linux-headers-5.13.0-52-generic/include",
                // "/usr/src/linux-headers-5.13.0-52-generic/include/uapi",
                // "/usr/src/linux-headers-5.13.0-52-generic/include/generated",
                // "/usr/src/linux-headers-5.13.0-52-generic/arch/x86/include",
                // "/usr/src/linux-headers-5.13.0-52-generic/arch/x86/include/generated"

                /* ----------- KERNEL DEFINITION ----------- */
                "${HOME}/linux/include",
                "${HOME}/linux/include/uapi",
                "${HOME}/linux/include/generated",
                "${HOME}/linux/arch/x86/include",
                "${HOME}/linux/arch/x86/include/generated"
            ],
            "defines": [
                "__GNUC__",
                "__KERNEL__",
                "MODULE"
            ],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "gnu17",
            "cppStandard": "gnu++14",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```
