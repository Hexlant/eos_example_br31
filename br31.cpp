#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>

using namespace std;
using namespace eosio;

class br31 : public eosio::contract {
  public:
    using contract::contract;

    void creategame(const account_name host, const asset& quantity)
    {
        games gametable(_self,_self);
        require_auth(host);
        auto existing = gametable.find( host );
        eosio_assert( existing == gametable.end(), "host already created a game" );
        gametable.emplace(_self,[&](auto& onegame)     
        {
            onegame.host = host;
            onegame.guest = 0;
            onegame.point = 0;
            onegame.turn = host;
            onegame.deposit=quantity;
        });

        print(name{host} ," Created Game");
    }

    void joingame(const account_name host, const account_name guest, const asset& quantity)
    {
        games gametable(_self,_self);
        require_auth(guest);
        auto existing = gametable.find( host );
        eosio_assert( existing != gametable.end(), "host didn't create a game" );
        eosio_assert( existing->guest == 0, "Another guest has already joined " );

        gametable.modify( existing, 0, [&]( auto& onegame ) {
            onegame.guest = guest;
        });

        print(name{guest}," joined", name{host},"Game");        
    }

    void rolldice (const account_name host, const account_name player) {
        require_auth(player);
        games gametable(_self,_self);
        auto existing = gametable.find( host );
        eosio_assert( existing != gametable.end(), "host didn't create a game" );
        eosio_assert( existing->turn == player, "it is not player’s turn" );
        
        // Make Random Number
        uint64_t number = rand();
        // Accumulate game point
        auto point = existing->point + number;
        print(name{player} ," rolled dice! number : ", number, " total score : ", point,);

        //Update game status
        gametable.modify( existing, 0, [&]( auto& onegame ) {
            // trun change
            onegame.turn = (player == host ? onegame.guest : host);
            onegame.point = point;
        });

        // Check game over
        if(point >= 31) {
            // current player lose.
            account_name winner = (player == host ? existing->guest : host);
            print(name{winner} ," is Win" );         

            // Remove game
            gametable.erase(existing);
        }
        
    }

    void deletegame(const account_name host) {
        games gametable(_self,_self);

        print(name{host} ,"'s  game delete" );      
        // Delete Game
        auto onegame = gametable.find(host);
        gametable.erase(onegame);
    }

    uint8_t rand() { 
        checksum256 result;
        auto mixedBlock = tapos_block_prefix() * tapos_block_num();

        const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
        sha256( (char *)mixedChar, sizeof(mixedChar), &result);
        const char *p64 = reinterpret_cast<const char *>(&result);

        uint8_t r = (abs((int8_t)p64[0]) % (10)) + 1;
        return r;
    }

  private:

    /// @abi table game i64
    struct game{
        account_name host;
        account_name guest;
        uint8_t point;
        asset deposit;
        account_name turn;
        uint64_t primary_key() const {return host;}
        EOSLIB_SERIALIZE(game,(host)(guest)(point)(deposit)(turn))
    };

    typedef multi_index<N(game),game> games;

};

EOSIO_ABI( br31, (creategame) (joingame) (rolldice) (deletegame) )
