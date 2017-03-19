# MiniNez3

## Requirement
Need to install cmake
### OSX
```
brew install cmake
```
### Ubuntu
```
sudo apt-get install cmake
```

## Build
You'll have to make a few steps. Type in:
```
  $ mkdir build
  $ cd build
  $ cmake ..
  $ make
  $ execute ./build/mininez
```

## Execution
You can execute sample as follows:
```
  $ ./build/mininez -g sample/bytecode/math.bin -i sample/input/math.txt -t tree
```

You can get the AST as follows:
```
#Sub[
  $left=#Add[
    $left=#Integer['1']
    $right=#Div[
      $left=#Mul[
        $left=#Integer['2']
        $right=#Integer['4']
      ]
      $right=#Integer['3']
    ]
  ]
  $right=#Integer['5']
]
```

Math grammar:
```
/* Start Point */
File        = Expression .*

/* Code Layout */
_           = S*
S           = [ \t]
"+"         = '+' _
"-"         = '-' _
"*"         = '*' _
"/"         = '/' _
"%"         = '%' _
"("         = '(' _
")"         = ')' _

/* Expression */
Expression  = Sum
Sum         = Product {$left ("+" #Add / "-" #Sub) $right(Product) }*
Product     = Value {$left ("*" #Mul / "/" #Div / "%" #Mod) $right(Value) }*
Value       = { [0-9]+ #Integer } _
            / { [A-Za-z0-9_]+ #Variable } _
            / "(" Expression ")"
```

Math example:
```
1+2*4/3-5
```
