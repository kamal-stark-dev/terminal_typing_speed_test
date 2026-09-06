# Terminal Typing Speed Test 

```
                         main()
                           │
                           ▼
                  Initialize terminal
                           │
                           ▼
                    Show welcome
                           │
                           ▼
                    while(play)
                           │
                           ▼
                  Pick random passage
                           │
                           ▼
                       runTest()
                           │
            ┌──────────────┼──────────────┐
            │              │              │
            ▼              ▼              ▼
       raw terminal      select()       timer
            │              │              │
            └──────────────┼──────────────┘
                           ▼
                      read input
                           │
                 ┌─────────┼─────────┐
                 ▼         ▼         ▼
              normal    backspace  Ctrl+C
                 │         │         │
                 ▼         ▼         ▼
              typed[]    remove    abort
                 │
                 ▼
            redraw screen
                 │
                 ▼
        60 sec / passage done
                 │
                 ▼
           return results
                 │
                 ▼
          countCorrect()
                 │
                 ▼
          calculateWpm()
                 │
                 ▼
          calculate accuracy
                 │
                 ▼
            printStats()
                 │
                 ▼
           printRating()
                 │
                 ▼
          askPlayAgain()
                 │
          ┌──────┴──────┐
          ▼             ▼
         yes            no
          │             │
          │             ▼
          │           exit
          │
          └──────→ next test
```
