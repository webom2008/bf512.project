.GLOBAL  _adi_insufficient_tls_slots;          // Termination address

.SECTION/program

_adi_insufficient_tls_slots: 
            JUMP _adi_insufficient_tls_slots;        // Stay put
