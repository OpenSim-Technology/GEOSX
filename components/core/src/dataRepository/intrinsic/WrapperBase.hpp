/*
 * DataObjectBase.hpp
 *
 *  Created on: Jun 17, 2016
 *      Author: rrsettgast
 */

#ifndef COMPONENTS_CORE_SRC_DATAREPOSITORY_DATAOBJECTBASE_HPP_
#define COMPONENTS_CORE_SRC_DATAREPOSITORY_DATAOBJECTBASE_HPP_

#include <string>
#include <memory>

namespace asctoolkit
{
namespace sidre
{
class DataView;
}
}

namespace geosx {
namespace dataRepository
{


template< typename T > class Wrapper;

class WrapperBase
{
public:

  /*!
   * \brief default destuctor
   */
  virtual ~WrapperBase();

  /*!
   *
   * @param name name of the object
   * \brief constructor
   */
  explicit WrapperBase( std::string const & name );


  WrapperBase( WrapperBase&& source );


  /*!
   *
   * @return type_info of the DataObject
   */
  virtual std::type_info const & get_typeid() const = 0 ;


  virtual bool empty() const = 0;
  virtual std::size_t size( ) const = 0;
  virtual void reserve( std::size_t new_cap ) = 0;
  virtual std::size_t capacity() const = 0;
  virtual std::size_t max_size() const = 0;
  virtual void clear() = 0 ;
  virtual void insert() = 0;
  /*
  iterator erase( iterator pos );
  iterator erase( const_iterator pos );
  iterator erase( const_iterator first, const_iterator last );
  size_type erase( const key_type& key );

  iterator erase( const_iterator pos );
  iterator erase( iterator first, iterator last );
  iterator erase( const_iterator first, const_iterator last );

  void swap( unordered_map& other );
  void swap( vector& other );
*/

  virtual void resize( std::size_t newsize ) = 0;



  template< typename T >
  static std::unique_ptr<WrapperBase> Factory( std::string const & name )
  {
    return std::move(std::make_unique<Wrapper<T> >( name ) );
  }

  template< typename T >
  Wrapper<T>& getObject()
  {
    return dynamic_cast<Wrapper<T>&>(*this);
  }

  template< typename T >
  Wrapper<T> const & getObject() const
  {
    return dynamic_cast<Wrapper<T> const &>(*this);
  }



  template< typename T >
  typename Wrapper<T>::const_rtype getObjectData() const
  { return (dynamic_cast<Wrapper<T> const &>(*this)).getObjectData(); }

  template< typename T >
  typename Wrapper<T>::rtype getObjectData()
  { return (dynamic_cast<Wrapper<T> &>(*this)).getObjectData(); }
//  { return const_cast<typename DataObject<T>::rtype>( const_cast<DataObjectBase const *>(this)->getObjectData<T>() ); }


private:
  std::string m_name;

  asctoolkit::sidre::DataView* m_sidreView;

  WrapperBase() = delete;
  WrapperBase( WrapperBase const & ) = delete;
  WrapperBase& operator=( WrapperBase const & ) = delete;
  WrapperBase& operator=( WrapperBase&& ) = delete;

};

}
} /* namespace geosx */

#endif /* COMPONENTS_CORE_SRC_DATAREPOSITORY_DATAOBJECTBASE_HPP_ */
