# 01 module_params

## Usage

Use `make` to compile the default target `module`.

## Result

If no parameters are provided, the defaults will be used:

```bash
$ sudo insmod module_params.ko
$ sudo rmmod module_params
$ dmesg
[...]
[ 5932.185578] module_params is loaded!
[ 5932.185581] The *mint* parameter: 9
[ 5932.185582] The *mchar* parameter: char
[ 5932.185583] The *marr* parameter: 0, 1
[ 5948.602527] module_params is unloaded.
```

`mint` and `mchar` are both with permission `S_IRUGO`, read-only for user and group. But `marr` is `S_IRUSR | S_IWUSR`, which means this variable has `write` and `read` permission. But with `root` permission, you can do anything you want even it has read-only permission. If parameters are provided, the defaults will be used:

```bash
$ sudo insmod module_params.ko mint=1 mchar="newchar" marr=66,99
$ sudo rmmod module_params
$ dmesg
[...]
[ 6539.808432] module_params is loaded!
[ 6539.808435] The *mint* parameter: 1
[ 6539.808436] The *mchar* parameter: newchar
[ 6539.808437] The *marr* parameter: 66, 99
[ 6547.390130] module_params is unloaded.
```
