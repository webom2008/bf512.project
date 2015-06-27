.GLOBAL  _adi_uninitialized_tls_slot;          // Termination address

.SECTION/program

_adi_uninitialized_tls_slot: 
            JUMP _adi_uninitialized_tls_slot;        // Stay put
