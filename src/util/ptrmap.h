/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPTRMAP_H
#define BTPTRMAP_H

#include <ktorrent_export.h>
#include <map>

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief Map of pointers with an optional auto-delete feature.
 *
 * When autodelete is on, every time we remove something from the map, the data
 * will be deleted.
 */
template<class Key, class Data>
class PtrMap
{
    bool autodel;
    std::map<Key, Data *> pmap;

public:
    /*!
     * Constructor.
     * \param autodel Whether or not to enable auto deletion
     */
    PtrMap(bool autodel = false)
        : autodel(autodel)
    {
    }

    /*!
     * Destructor. Will delete all objects, if auto deletion is on.
     */
    virtual ~PtrMap()
    {
        clear();
    }

    /*!
     * Return the number of key data pairs in the map.
     */
    [[nodiscard]] unsigned int count() const
    {
        return pmap.size();
    }

    /*!
     * Enable or disable auto deletion.
     * \param yes Enable if true, disable if false
     */
    void setAutoDelete(bool yes)
    {
        autodel = yes;
    }

    using iterator = typename std::map<Key, Data *>::iterator;
    using const_iterator = typename std::map<Key, Data *>::const_iterator;

    iterator begin()
    {
        return pmap.begin();
    }
    iterator end()
    {
        return pmap.end();
    }

    [[nodiscard]] const_iterator begin() const
    {
        return pmap.begin();
    }
    [[nodiscard]] const_iterator end() const
    {
        return pmap.end();
    }

    /*!
     * Remove all objects, will delete them if autodelete is on.
     */
    void clear()
    {
        if (autodel) {
            for (iterator i = pmap.begin(); i != pmap.end(); ++i) {
                delete i->second;
                i->second = nullptr;
            }
        }
        pmap.clear();
    }

    /*!
     * Insert a key data pair.
     * \param k The key
     * \param d The data
     * \param overwrite Whether or not to overwrite
     * \return true if the insertion took place
     */
    bool insert(const Key &k, Data *d, bool overwrite = true)
    {
        iterator itr = pmap.find(k);
        if (itr != pmap.end()) {
            if (overwrite) {
                if (autodel)
                    delete itr->second;
                itr->second = d;
                return true;
            } else {
                return false;
            }
        } else {
            pmap[k] = d;
            return true;
        }
    }

    /*!
     * Find a key in the map and returns it's data.
     * \param k The key
     * \return The data of the key, 0 if the key isn't in the map
     */
    Data *find(const Key &k)
    {
        iterator i = pmap.find(k);
        return (i == pmap.end()) ? nullptr : i->second;
    }

    /*!
     * Find a key in the map and returns it's data.
     * \param k The key
     * \return The data of the key, 0 if the key isn't in the map
     */
    [[nodiscard]] const Data *find(const Key &k) const
    {
        const_iterator i = pmap.find(k);
        return (i == pmap.end()) ? nullptr : i->second;
    }

    /*!
     * Check to see if a key is in the map.
     * \param k The key
     * \return true if it is part of the map
     */
    [[nodiscard]] bool contains(const Key &k) const
    {
        const_iterator i = pmap.find(k);
        return i != pmap.end();
    }

    /*!
     * Erase a key from the map. Will delete
     * the data if autodelete is on.
     * \param key The key
     * \return true if an erase took place
     */
    virtual bool erase(const Key &key)
    {
        iterator i = pmap.find(key);
        if (i == pmap.end())
            return false;

        if (autodel)
            delete i->second;
        pmap.erase(i);
        return true;
    }

    /*!
        Erase an iterator from the map.
    */
    iterator erase(iterator i)
    {
        iterator j = i;
        ++j;
        pmap.erase(i);
        return j;
    }
};

}

#endif
