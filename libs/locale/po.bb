// Polish
// Contributors: 
//  - GPT4.0
//  - Emmanouil Krasanakis (maniospas) - editing

!macro{wektor::zero} as {vector::zero}
!macro{wektor} as {vector}
!macro{lista} as {list}
!macro{wypisz} as {print}
!macro{czytaj} as {read}
!macro{całkowita} as {int}
!macro{łańcuch} as {str}
!macro{dodaj} as {add}
!macro{odejmij} as {sub}
!macro{pomnóż} as {mul}
!macro{podziel} as {div}
!macro{potęga} as {pow}
!macro{logarytm} as {log}
!macro{rzeczywista} as {float}
!macro{boolowski} as {bool}
!macro{wywołanie} as {call}
!macro{grafika} as {graphics}
!macro{jako} as {as}
!macro{długość} as {len}
!macro{dopóki(@expr)} as {while(@expr)}
!macro{jeśli(@expr)} as {if(@expr)}
!macro{przechwyć(@expr)} as {catch(@expr)}
!macro{w} as {in}
!macro{w zakresie} as {in range}
!macro{zakres} as {range}
!macro{iteruj} as {iter}
!macro{wykonaj} as {do}
!macro{zwróć} as {return}
!macro{usuń} as {pop}
!macro{wstaw} as {push}
!macro{następny} as {next}
!macro{plik} as {file}
!macro{wyczyść} as {clear}
!macro{przenieś} as {move}
!macro{!dołącz} as {!include}
!macro{!makro{@expr} jako {@impl}} as {!macro{@expr} as {@impl}}
!macro{!lokalne{@expr} jako {@impl}} as {!local{@expr} as {@impl}}
!macro{!z} as {!of}
!macro{!napis} as {!stringify}
!macro{!symbol} as {!symbol}
!macro{!kompilacji} as {!comptime}
!macro{to} as {this}
!macro{losowy} as {random}
!macro{czas} as {time}

// standard library
!macro{!przestrzeń_nazw} as {!namespace}
!macro{!zmienna} as {!var}
!macro{!anonimowy} as {!nameless}
!macro{!z} as {!with}
!macro{!zbierz} as {gather}
!macro{przynieś} as {yield}
!macro{domyślny} as {default}
!macro{upewnij} as {assert}

!macro{nowy{@block}} as {new{@block}}
!macro{język} as {bb}
!macro{system_operacyjny} as {os}
!macro{przenieś} as {transfer}
!macro{tekst} as {string}
!macro{połącz} as {join}
!macro{zaczyna} as {starts}
!macro{kończy} as {ends}
!macro{indeks} as {index}
!macro{podziel} as {split}
!macro{pozycja} as {pos}
!macro{pamięć} as {memory}

!macro{baza_danych} as {db}
!macro{tabela} as {table}
!macro{zatwierdź} as {commit}
!macro{transakcja} as {transaction}
!macro{uruchom} as {run}

!macro{nauka} as {sci}
!macro{wykres} as {plot}
!macro{wykresy} as {plots}
!macro{Wykres} as {Plot}
!macro{pokaż} as {show}
!macro{szerokość} as {width}
!macro{wysokość} as {height}
!macro{kąt} as {angle}
!macro{kolor} as {color}
!macro{tytuł} as {title}
!macro{czcionka} as {font}
!macro{osie} as {axes}
!macro{cieniowanie} as {shade}
!macro{tekst} as {text}

!macro{dziennik} as {logger}
!macro{ok} as {ok}
!macro{informacja} as {info}
!macro{błąd} as {fail}
!macro{ostrzeż} as {warn}
