#pragma once
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>

/// <summary>
/// raw virtual instruction information extracted using lex and yacc...
/// </summary>
struct _vinstr_meta
{
    /// <summary>
    /// virtual instruction name...
    /// </summary>
    std::string name;

    /// <summary>
    /// if the virtual instruction has a second operand or not (imm)...
    /// </summary>
    bool has_imm;

    /// <summary>
    /// the imm if any...
    /// </summary>
    std::uintptr_t imm;
};

/// <summary>
/// raw label containing raw virtual instruction data extracted using lex and yacc...
/// </summary>
struct _vlabel_meta
{
    /// <summary>
    /// label name...
    /// </summary>
    std::string label_name;

    /// <summary>
    /// vector of raw virtual instruction data...
    /// </summary>
    std::vector< _vinstr_meta > vinstrs;
};

/// <summary>
/// used for parse_t::for_each...
/// </summary>
using callback_t = std::function< bool( _vlabel_meta * ) >;

/// <summary>
/// this singleton class contains all the information for parsed virtual instructions...
/// </summary>
class parse_t
{
  public:
    /// <summary>
    /// gets the one and only instance of this class...
    /// </summary>
    /// <returns>returns a pointer to the one and only instance of this class...</returns>
    static auto get_instance() -> parse_t *;

    /// <summary>
    /// used by yacc file to add new labels...
    /// </summary>
    /// <param name="label_name">label name, no pass by reference since a new std::string object must be
    /// created...</param>
    void add_label( std::string label_name );

    /// <summary>
    /// used by yacc file to add new virtual instruction with no imm...
    /// </summary>
    /// <param name="vinstr_name">virtual instruction name, no pass by reference since a new std::string object must be
    /// created...</param>
    void add_vinstr( std::string vinstr_name );

    /// <summary>
    /// used by yacc file to add new virtual instruction with an imm...
    /// </summary>
    /// <param name="vinstr_name">virtual instruction name, no pass by reference since a new std::string object must
    /// be created...</param>
    /// <param name="imm_val">imm value...</param>
    void add_vinstr( std::string vinstr_name, std::uintptr_t imm_val );

    /// <summary>
    /// used to loop over every single label...
    /// </summary>
    /// <param name="callback">lambda to call back given the label structure...</param>
    /// <returns>returns true if all labels were looped through...</returns>
    bool for_each( callback_t callback );

  private:
    /// <summary>
    /// vector of raw virtual labels...
    /// </summary>
    std::vector< _vlabel_meta > virt_labels;

    /// <summary>
    /// default constructor... private...
    /// </summary>
    parse_t();
};