#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdlib.h>

// **************************************************
// Verbs
// Edit this to add new verbs to the parser, the enum and
// char array need to match each other

typedef enum {
    VERB_UNKNOWN = -1,
    VERB_EXITS,
    VERB_GO,
    VERB_LOOK,
    VERB_TAKE,
    VERB_OPEN,
    VERB_INVENTORY,
    VERB_TALK,
    VERB_USE,
    VERB_COUNT
} Verb;

const char *verb_names[] = {
    "exits",
    "go",
    "look",
    "take",
    "open",
    "inventory",
    "talk",
    "use"
};

// **************************************************
// Nouns
// Edit this to add new nouns

typedef enum {
    NOUN_DOOR,
    NOUN_KEY,
    NOUN_CHEST,
    NOUN_ROOM,
    NOUN_COUNT,
    NOUN_UNKNOWN = -1
} Noun;

const char *noun_names[] = {
    "door", "key", "chest", "room"
};

// Don't edit this

typedef struct {
    Verb verb;
    Noun noun;
    char raw_noun[32];
} ParsedVerb;

// **************************************************
// Directions
// Only change this if you know why you would want to

typedef enum {
    DIR_UNKNOWN = -1,
    DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST, DIR_UP, DIR_DOWN,
    DIR_COUNT
} Direction;

const char* dir_names[] = {
    "North", "South", "East", "West", "Up", "Down"
};

// **************************************************
// Items
// Change this to add more items

typedef enum {
    ITEM_UNKNOWN = -1, ITEM_KEY, ITEM_LAMP, ITEM_NOTE, ITEM_COMPUTER,
    ITEM_COUNT
} ItemType;

const char *item_names[] = {
    "key", "lamp", "note", "computer"
};

// Don't edit this

typedef struct {
    char *description;
    ItemType type;
    int is_pickable;
    int is_usable;
    int is_collected;
    int is_interactive;
    void (*action)(void);
} Item;

// **************************************************
// Dialog & NPC
// Don't edit this

typedef struct {
    const char *player_choice;
    int next_node;
    void (*effect)(void);   // Callback
} DialogOption;

typedef struct {
    const char *npc_line;
    const DialogOption *options;
    int num_options;
} DialogNode;

typedef struct {
    const char *name;
    const char *description;
    const char *location;
    DialogNode *dialog;
} NPC;

// **************************************************
// Room

// These control how many of each thing the game will have
#define MAX_ROOM_ITEMS 4
#define MAX_ROOM_OBJECTS 4
#define MAX_ROOM_DESTINATIONS 4
#define MAX_ROOMS 10

// Don't edit these
typedef struct {
    char *description;
    int is_locked;
    int blocked;
    int goes_to;
} Exit;

typedef enum {
    ROOM_NORMAL,
    ROOM_SELECTOR
} RoomType;

typedef struct {
    const char* name;
    const char* description;
    RoomType room_type;
    Exit exits[DIR_COUNT];  // index of other rooms, or -1 for none
    Item items[MAX_ROOM_ITEMS];
    int num_items;
    const char *destination_names[4];
    int destination_ids[4];
    int num_destinations;
    NPC *npc;
} Room;

// **************************************************
// The game world
// Don't edit
Room world[MAX_ROOMS];
ItemType inventory[ITEM_COUNT];
int current_room = 0;

// **************************************************
// NPC Dialog
// You need this for each NPC and its dialog
const DialogOption bob1_options[] = {
    {"What?", 1}
};

const DialogOption bob2_options[] = {
    {"What?", 2}
};

const DialogOption bob3_options[] = {
    {"What?", 3}
};

const DialogOption bob4_options[] = {
    {"Yes!", 4}
};

const DialogOption bob5_options[] = {
    {"What?", 5}
};

const DialogOption bob6_options[] = {
    {"Um? I have to go now?", -1},
    {"What?", 4}
};

DialogNode bob_dialog[] = {
    {"What country you from?", bob1_options, 1},
    {"What ain't no country I know, they speak English in What?", bob2_options, 1},
    {"English-non-advertiser-friendly-language, do you speak it?", bob3_options, 1},
    {"Then you understand what I'm sayin'?", bob4_options, 1},
    {"Now describe what a nice cup of tea looks like?", bob5_options, 1},
    {"Say what again... Nah only joking", bob6_options, 2}
};

// **************************************************
// NPC
// Create one of these for each NPC

NPC bob = {
    .name = "Bob",
    .description = "A great green gorilla",
    .location = "sitting in the corner of the room, eating a biscuit",
    .dialog = bob_dialog
};

// **************************************************
// Parsing routines
// Don't edit this part

Verb parse_verb(const char *word) {
    for (int i = 0; i < VERB_COUNT; i++) {
        if (strcasecmp(word, verb_names[i]) == 0) {
            return (Verb)i;
        }
    }
    return VERB_UNKNOWN;
}

Noun parse_noun(const char *word) {
    for (int i = 0; i < NOUN_COUNT; i++) {
        if (strcasecmp(word, noun_names[i]) == 0) {
            return (Noun)i;
        }
    }
    return NOUN_UNKNOWN;
}

Direction parse_direction(const char *word) {
    for (int i = 0; i < DIR_COUNT; i++) {
        if (strcasecmp(word, dir_names[i]) == 0) {
            return (Direction)i;
        }
    }
    return DIR_UNKNOWN;
}

ItemType parse_item(const char *word)
{
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (strcasecmp(word, item_names[i]) == 0) {
            return (ItemType)i;
        }
    }
    return ITEM_UNKNOWN;
}

// ---------- Command Parser ----------
ParsedVerb parse_text(char* input) {
    ParsedVerb result;
    char parsed_token[32] = "";
    result.verb = VERB_UNKNOWN;
    result.noun = NOUN_UNKNOWN;

    char* token = strtok(input, " \t\n");
    if (token) {
        strncpy(parsed_token, token, sizeof(parsed_token) - 1);
        parsed_token[sizeof(parsed_token) - 1] = '\0';

        result.verb = parse_verb(parsed_token);

        token = strtok(NULL, " \t\n");

        if (token) {
            strncpy(parsed_token, token, sizeof(parsed_token) - 1);
            strncpy(result.raw_noun, token, sizeof(result.raw_noun) - 1);
            parsed_token[sizeof(parsed_token) - 1] = '\0';
            result.raw_noun[sizeof(parsed_token) - 1] = '\0';
            result.noun = parse_noun(parsed_token);
        }
    }

    return result;
}

// **************************************************
// Inventory
// Don't edit this part

int player_has_item(ItemType item) {
    return inventory[item];
}

void player_pick_up(ItemType item)
{
    inventory[item] = 1;
}

void player_use_item(ItemType item)
{
    inventory[item] = 0;
}

void player_list_inventory()
{
    int nothing = 1;
    printf ("You are carrying: ");
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (inventory[i]) {
            nothing = 0;
            printf ("%s ", item_names[i]);
        }
    }
    if (nothing) printf ("nothing!");
    printf ("\n");
}

// **************************************************
// Basic game logic, don't edit

void describe_exits() {
    Room* r = &world[current_room];

    printf ("Room exits are:");
    for (int i = 0; i < DIR_COUNT; i++) {
        if (!r->exits[i].blocked)
            printf ("%s ", dir_names[i]);
        if (r->exits[i].is_locked && inventory[ITEM_KEY] == 0)
            printf ("(locked) ");
    }
    printf ("\n");
}

void handle_selector_room(const Room *room)
{
    while (1) {
        printf ("Where would you like to go?\n");
        printf ("  0 - Leave room\n");
        for (int i = 0; i < room->num_destinations; i++) {
            printf ("  %d - %s\n", i+1, room->destination_names[i]);
        }

        printf ("Choice >");
        char input[4];
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input);

        if (choice == 0) {
            current_room = room->exits[0].goes_to;
            break;
        }
        if (choice >= 1 && choice <= room->num_destinations) {
            current_room = room->destination_ids[choice-1];
            break;
        }

        printf ("Can't go there!\n");
    }
}

void describe_room() {
    Room* r = &world[current_room];
    printf("%s\n%s\n", r->name, r->description);

    if (r->room_type == ROOM_SELECTOR) {
        handle_selector_room(r);
        describe_room();
        return;
    }

    if (r->num_items > 0) {
        printf ("You can see\n");
        for (int i = 0; i < r->num_items; i++) {
            if (r->items[i].is_collected) continue;

            printf("  %s\n", r->items[i].description);
        }
    }
    if (r->npc) {
        printf ("%s, %s is %s\n", r->npc->name, r->npc->description, r->npc->location);
    }
}

int go_direction(Direction direction) {
    Room* r = &world[current_room];

    if (r->exits[direction].blocked) {
        printf ("That way is blocked\n");
        return -1;
    }


    if (r->exits[direction].is_locked && (inventory[ITEM_KEY] == 0)) {
        printf ("The door is locked\n");
        return -1;
    }

    printf ("You go through the door.\n");
    return r->exits[direction].goes_to;
}

void run_dialog(const DialogNode *dialog)
{
    int current = 0;

    while (current >= 0) {
        const DialogNode *node = &dialog[current];

        printf("\n%s\n", node->npc_line);

        for (int i = 0; i < node->num_options; i++) {
            printf ("  %d - %s\n", i+1, node->options[i].player_choice);
        }

        printf ("Choice >");
        char input[4];
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input) -1;

        if (choice >= 0 && choice < node->num_options) {
            const DialogOption *opt = &node->options[choice];
            if (opt->effect) opt->effect();
            current = opt->next_node;
        } else {
            printf ("I don't understand\n");
        }
    }
}

// **************************************************
// Interactive
// These are mini programs that interactive items can execute

void computer_test()
{
    int quit = 0;
    while (!quit) {
        printf ("Welcome to the secret computer!\n");
        printf ("In here we can do anything\n");
        printf ("But right now, I don't work.\n");
        printf ("+++ OUT OF CHEESE ERROR, REDO FROM START +++\n");
        quit = 1;
    }
}

// **************************************************
// World initialisation
// Edit this

void init_world() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        world[i].num_destinations = 0;
        world[i].num_items = 0;
        world[i].npc = 0;
        world[i].room_type = ROOM_NORMAL;
        for (int j = 0; j < DIR_COUNT; j++)
            world[i].exits[j].blocked = 1;
    }

    // Room 0 - Hallway
    world[0].name = "Hallway";
    world[0].description = "A narrow hallway with a door to the east.";
    world[0].exits[DIR_NORTH].blocked = 1;
    world[0].exits[DIR_EAST] = (Exit){"The East Door", 1, 0, 1};
    world[0].exits[DIR_WEST] = (Exit){"Magic Teleporter", 0, 0, 2};
    world[0].items[0] = (Item){
        .description = "A rusty old key",
        .type = ITEM_KEY,
        .is_pickable = 1,
        .is_usable = 0,
        .is_collected = 0,
        .is_interactive = 0,
        .action = 0
    };
    world[0].num_items = 1;

    // Room 1 - Storage
    world[1].name = "Storage Room";
    world[1].description = "Dusty old room filled with boxes.";
    world[1].exits[DIR_WEST] = (Exit){"The West Door", 1, 0, 0};
    world[1].items[0] = (Item){
        .description = "A small oil lamp",
        .type = ITEM_LAMP,
        .is_pickable = 1,
        .is_usable = 1,
        .is_collected = 0,
        .is_interactive = 0,
        .action = 0
    };
    world[1].npc = &bob;
    world[1].num_items = 1;

    // Room 2 - Magic teleporter room
    world[2].name = "Magic Teleporter";
    world[2].description = "In the middle of the room is a strange glowing orb showing images of far away places.";
    world[2].exits[DIR_EAST] = (Exit){"Door to hallway", 0, 0, 0};
    world[2].num_items = 0;
    world[2].room_type = ROOM_SELECTOR;
    world[2].destination_names[0] = "Secret room 1";
    world[2].destination_names[1] = "Secret room 2";
    world[2].destination_ids[0] = 3;
    world[2].destination_ids[1] = 4;
    world[2].num_destinations = 2;

    world[3].name = "Secret room 1";
    world[3].description = "A secret room you can't get into normally";
    world[3].exits[DIR_EAST] = (Exit){"Door to hallway", 0, 0, 0};
    world[3].items[0] = (Item){
        .description = "A computer terminal",
        .type = ITEM_COMPUTER,
        .is_pickable = 1,
        .is_usable = 0,
        .is_collected = 0,
        .is_interactive = 1,
        .action = &computer_test
    };
    world[3].num_items = 1;

    world[4].name = "Secret room 1";
    world[4].description = "A secret room you can't get into normally";
    world[4].exits[DIR_EAST] = (Exit){"Door to hallway", 0, 0, 0};
    world[4].num_items = 0;
}

// ---------- Main Game Loop ----------
// Edit parts of this
int main() {
    char input[64];

    init_world();

    printf("Welcome to the Adventure!\n");

    while (1) {
        describe_room();
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        ParsedVerb cmd = parse_text(input);

        switch (cmd.verb) {
            case VERB_UNKNOWN:
                printf ("I don't understand %s\n", input);
                break;
            case VERB_EXITS: describe_exits(); break;
            case VERB_GO: {
                Direction dir = parse_direction(cmd.raw_noun);
                int new_room = go_direction(dir);
                if (new_room != -1) {
                    current_room = new_room;
                }
            } break;
            case VERB_TAKE: {
                ItemType item = parse_item(cmd.raw_noun);
                if (item == ITEM_UNKNOWN) {
                    printf ("Can't pick that up!\n");
                    break;
                }
                Room* r = &world[current_room];
                for (int i = 0; i < r->num_items; i++) {
                    if (r->items[i].type == item && r->items[i].is_pickable && !(r->items[i].is_collected)) {
                        player_pick_up(item);
                        r->items[i].is_collected = 1;
                    }
                }
            } break;
            case VERB_INVENTORY:
                player_list_inventory();
                break;
            case VERB_TALK: {
                Room* r = &world[current_room];
                if (r->npc) {
                    run_dialog(r->npc->dialog);
                } else {
                    printf ("You mutter to nobody in particular\n");
                }
            } break;
            case VERB_USE: {
                printf ("Use!\n");
                ItemType item = parse_item(cmd.raw_noun);
                if (item == ITEM_UNKNOWN) {
                    printf ("Can't use that!\n");
                    break;
                }
                Room* r = &world[current_room];
                for (int i = 0; i < r->num_items; i++) {
                    if (r->items[i].type == item && r->items[i].is_interactive && r->items[i].action) {
                        r->items[i].action();
                    }
                }
            } break;
            default: break;
        }
    }

    return 0;
}
