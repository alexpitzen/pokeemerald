import re
import sys

changes = {}

full_evo_line = re.compile(
    r"[+\- ]\s*\[SPECIES_(?P<from>[^\]]*)]\s*= {{EVO_(?P<method>\w*), (ITEM_)?(?P<param>\w*), SPECIES_(?P<to>\w*)}"
)
partial_evo_line = re.compile(
    r"[+\- ][^=]*{EVO_(?P<method>\w*), (ITEM_)?(?P<param>\w*), SPECIES_(?P<to>\w*)}"
)
learn_pkmn_line = re.compile(r".* s(?P<pkmn>\w+)LevelUpLearnset")
learn_move_line = re.compile(
    r"[+\-]\s*LEVEL_UP_MOVE\(\s*(?P<level>\d*),\s*MOVE_(?P<move>\w+)\)"
)


class Change:
    def __init__(self):
        self.evo = {}
        self.moves = []

    def __repr__(self):
        return f"{{e: {self.evo}; m: {self.moves}}}"


class EvoChange:
    def __init__(self):
        self.method_before = None
        self.method_after = None
        self.param_before = None
        self.param_after = None

    def methodChanged(self):
        return self.method_before != self.method_after

    def paramChanged(self):
        return self.param_before != self.param_after

    def __repr__(self):
        if self.methodChanged() and self.paramChanged():
            return f"{self.method_before} -> {self.method_after}; {self.param_before} -> {self.param_after}"
        elif self.paramChanged():
            return f"{self.param_before} -> {self.param_after}"


class MoveChange:
    def __init__(self):
        self.level_before = None
        self.level_after = None
        self.move_before = None
        self.move_after = None

    def moveChanged(self):
        return self.move_before != self.move_after

    def levelChanged(self):
        return self.level_before != self.level_after

    def __repr__(self):
        if self.moveChanged():
            return f"{self.move_before} -> {self.move_after}; {self.level_before} -> {self.level_after}"
        return f"{self.move_before} {self.level_before} -> {self.level_after}"


with open(sys.argv[1], "r") as f:
    last_neutral = None
    last_old = None
    last_new = None
    while line := f.readline():
        if match := full_evo_line.match(line):
            if line[0] == " ":
                last_neutral = match
                last_old = None
                last_new = None
                continue
            elif line[0] == "-":
                last_old = match
            elif line[0] == "+":
                last_new = match
            else:
                print(f"??? {line}")
                continue

            pkmn = match.group("from")
        elif match := partial_evo_line.match(line):
            if line[0] == " ":
                continue
            elif line[0] == "-":
                if last_old is not None:
                    pkmn = last_old.group("from")
                else:
                    pkmn = last_neutral.group("from")
            elif line[0] == "+":
                if last_new is not None:
                    pkmn = last_new.group("from")
                else:
                    pkmn = last_neutral.group("from")
            else:
                print(f"???? {line}")
                continue
        else:
            continue

        pkmn = pkmn.replace("_", "").title()
        pkmn_to = match.group("to").title()
        method = match.group("method").lower()
        param = match.group("param").replace("_", " ").title()

        if pkmn not in changes:
            changes[pkmn] = Change()
        change = changes[pkmn]
        if pkmn_to not in change.evo:
            change.evo[pkmn_to] = EvoChange()
        evo_change = change.evo[pkmn_to]

        if line[0] == "-":
            evo_change.method_before = method
            evo_change.param_before = param
        elif line[0] == "+":
            evo_change.method_after = method
            evo_change.param_after = param

with open(sys.argv[2], "r") as f:
    pkmn = None
    while line := f.readline():
        if match := learn_pkmn_line.match(line):
            pkmn = match.group("pkmn").title()
            iRemoved = 0
            iAdded = 0
        elif match := learn_move_line.match(line):
            if pkmn not in changes:
                changes[pkmn] = Change()
            change = changes[pkmn]
            level = match.group("level")
            move = match.group("move").replace("_", " ").title()
            if line[0] == "-":
                if iRemoved >= iAdded:
                    moveChange = MoveChange()
                    change.moves.append(moveChange)
                else:
                    moveChange = change.moves[iRemoved]
                iRemoved += 1
                moveChange.move_before = move
                moveChange.level_before = level
            elif line[0] == "+":
                if iAdded >= iRemoved:
                    moveChange = MoveChange()
                    change.moves.append(moveChange)
                else:
                    moveChange = change.moves[iAdded]
                iAdded += 1
                moveChange.move_after = move
                moveChange.level_after = level

national = []
with open("national_unique.txt", "r") as f:
    while line := f.readline():
        national.append(line.rstrip())

n = 0
for pkmn_name in national:
    if pkmn_name == "Nidoran♀":
        pkmn = "Nidoranf"
    elif pkmn_name == "Nidoran♂":
        pkmn = "Nidoranm"
    elif pkmn_name == "Farfetch'd":
        pkmn = "Farfetchd"
    elif pkmn_name == "Mr. Mime":
        pkmn = "Mrmime"
    elif pkmn_name == "Ho-Oh":
        pkmn = "Hooh"
    else:
        pkmn = pkmn_name
    if pkmn not in changes:
        continue
    change = changes[pkmn]
    n += 1
    print(f"    * {pkmn_name}")
    for pkmn_to, evo in change.evo.items():
        print(f"        * Evolves into {pkmn_to} ", end="")
        if not evo.methodChanged() and evo.method_after == "level":
            print(f"at level ~~{evo.param_before}~~ -> {evo.param_after}")
        elif evo.method_before == "friendship" and evo.method_after == "level":
            print(f"~~with high friendship~~ -> at level {evo.param_after}")
        elif evo.method_before == "friendship_day" and evo.method_after == "item":
            print(f"with ~~high friendship during daytime~~ -> {evo.param_after}")
        elif evo.method_before == "friendship_night" and evo.method_after == "item":
            print(f"with ~~high friendship during nighttime~~ -> {evo.param_after}")
        elif evo.method_before == "trade" and evo.method_after == "level":
            print(f"~~when traded~~ -> at level {evo.param_after}")
        elif evo.method_before == "trade_item":
            print(f"~~when traded with {evo.param_before}~~ -> ", end="")
            if evo.method_after == "level":
                print(f"at level {evo.param_after}")
            elif evo.method_after == "item":
                print(f"with {evo.param_after}")
            else:
                raise Exception(f"Unhandled: {evo}")
        elif evo.method_before == "beauty" and evo.method_after == "level":
            print(f"~~with high beauty~~ -> at level {evo.param_after}")
        else:
            raise Exception(f"Unhandled: {evo}")
    for move in change.moves:
        if None in [
            move.move_after,
            move.move_before,
            move.level_after,
            move.level_before,
        ]:
            raise Exception(f"Bad move: {move}")
        print("        * Learns ", end="")
        if move.moveChanged() and move.levelChanged():
            print(
                f"~~{move.move_before} at {move.level_before}~~ -> {move.move_after} at {move.level_after}"
            )
        elif move.levelChanged():
            print(f"{move.move_after} at ~~{move.level_before}~~ -> {move.level_after}")
        else:
            raise Exception(f"unhandled: {move}")
if n != len(changes.keys()):
    raise Exception(f"{len(changes.keys())} changes but only {n} printed (probably a name/spelling mismatch between changes dict and national dex")
