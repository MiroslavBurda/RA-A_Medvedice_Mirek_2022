void Print() {
    printf("L: %d   R: %d  UltraUp: %i  Ultradown: %i \n", l, r, UltraUp, UltraDown);
    SerialBT.print("L: ");
    SerialBT.print(l);
    SerialBT.print("  R: ");
    SerialBT.print(r);
    SerialBT.print("  UltraUP: ");
    SerialBT.print(UltraUp);
    SerialBT.print("  UltraDown: ");
    SerialBT.print(UltraDown);
    SerialBT.print("  Min: ");
    SerialBT.println(Min);
    last_millis = millis();
}

void rkIr() { // prumerovani IR
    while (true) {
        k++;
        if (k == 30000)
            k = 0;
        switch (k % 4) {
        case 0:
            IrL[0] = rkIrLeft();
            IrR[0] = rkIrRight();
            break;
        case 1:
            IrL[1] = rkIrLeft();
            IrR[1] = rkIrRight();
            break;
        case 2:
            IrL[2] = rkIrLeft();
            IrR[2] = rkIrRight();
            break;
        case 3:
            IrL[3] = rkIrLeft();
            IrR[3] = rkIrRight();
            break;
        default:
            printf("Chyba v prikazu switch");
            break;
        }
        l = (IrL[0] + IrL[1] + IrL[2] + IrL[3]) / 4;
        r = (IrR[0] + IrR[1] + IrR[2] + IrR[3]) / 4;
        delay(10);
    }
}